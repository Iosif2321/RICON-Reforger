#!/usr/bin/env python3
"""Хост-эталон L0 для требований RICON; production-кодом не является."""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any


SPEED_OF_LIGHT_MPS = 299_792_458.0
BOLTZMANN_J_PER_K = 1.380649e-23
REFERENCE_TEMPERATURE_K = 290.0

P372_ENVIRONMENTS: dict[str, tuple[float, float] | None] = {
    "city": (76.8, 27.7),
    "residential": (72.5, 27.7),
    "rural": (67.2, 27.7),
    "quiet_rural": (53.6, 28.6),
    "none": None,
}

SPECIAL_FLOATS = {
    "NaN": math.nan,
    "Infinity": math.inf,
    "-Infinity": -math.inf,
}


class OracleError(ValueError):
    """Ошибка границы с машинно-проверяемой причиной."""

    def __init__(self, reason: str, message: str):
        super().__init__(message)
        self.reason = reason


def decode_special_floats(value: Any) -> Any:
    """Преобразовать строковые NaN/Inf из валидного JSON во float."""
    if isinstance(value, str) and value in SPECIAL_FLOATS:
        return SPECIAL_FLOATS[value]
    if isinstance(value, list):
        return [decode_special_floats(item) for item in value]
    if isinstance(value, dict):
        return {key: decode_special_floats(item) for key, item in value.items()}
    return value


def require_finite(value: Any, reason: str, name: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise OracleError(reason, f"{name} должно быть числом")
    result = float(value)
    if not math.isfinite(result):
        raise OracleError(reason, f"{name} должно быть конечным")
    return result


def require_bool(value: Any, reason: str, name: str) -> bool:
    if not isinstance(value, bool):
        raise OracleError(reason, f"{name} должно быть bool")
    return value


def distance_3d(tx_pos: Any, rx_pos: Any) -> float:
    if not isinstance(tx_pos, list) or not isinstance(rx_pos, list) or len(tx_pos) != 3 or len(rx_pos) != 3:
        raise OracleError("INVALID_COORDINATE", "позиции должны содержать по три координаты")
    tx = [require_finite(value, "INVALID_COORDINATE", "txPos") for value in tx_pos]
    rx = [require_finite(value, "INVALID_COORDINATE", "rxPos") for value in rx_pos]
    return math.sqrt(sum((a - b) ** 2 for a, b in zip(tx, rx, strict=True)))


def free_space_loss(distance_m: Any, frequency_mhz: Any) -> dict[str, float]:
    distance = require_finite(distance_m, "INVALID_COORDINATE", "distanceM")
    frequency = require_finite(frequency_mhz, "INVALID_FREQUENCY", "frequencyMHz")
    if distance < 0:
        raise OracleError("INVALID_COORDINATE", "distanceM не может быть отрицательной")
    if frequency <= 0:
        raise OracleError("INVALID_FREQUENCY", "frequencyMHz должна быть положительной")
    wavelength = SPEED_OF_LIGHT_MPS / (frequency * 1_000_000.0)
    effective_distance = max(distance, wavelength)
    loss = 20.0 * math.log10(4.0 * math.pi * effective_distance / wavelength)
    if not math.isfinite(loss):
        raise OracleError("NUMERIC_FAILURE", "FSPL не конечна")
    return {"lossDb": loss, "effectiveDistanceM": effective_distance}


def thermal_noise(bandwidth_hz: Any) -> float:
    bandwidth = require_finite(bandwidth_hz, "INVALID_BANDWIDTH", "bandwidthHz")
    if bandwidth <= 0:
        raise OracleError("INVALID_BANDWIDTH", "bandwidthHz должна быть положительной")
    result = 10.0 * math.log10(
        BOLTZMANN_J_PER_K * REFERENCE_TEMPERATURE_K * bandwidth * 1000.0
    )
    if not math.isfinite(result):
        raise OracleError("NUMERIC_FAILURE", "тепловой шум не конечен")
    return result


def system_noise(
    bandwidth_hz: Any,
    noise_figure_db: Any,
    environment_id: Any,
    frequency_mhz: Any,
) -> dict[str, float | bool]:
    thermal_dbm = thermal_noise(bandwidth_hz)
    noise_figure = require_finite(
        noise_figure_db, "INVALID_NOISE_FIGURE", "rxNoiseFigureDb"
    )
    frequency = require_finite(frequency_mhz, "INVALID_FREQUENCY", "frequencyMHz")
    if noise_figure < 0:
        raise OracleError("INVALID_NOISE_FIGURE", "rxNoiseFigureDb не может быть отрицательным")
    if frequency <= 0:
        raise OracleError("INVALID_FREQUENCY", "frequencyMHz должна быть положительной")
    if not isinstance(environment_id, str) or environment_id not in P372_ENVIRONMENTS:
        raise OracleError("UNKNOWN_NOISE_ENVIRONMENT", "неизвестная категория внешнего шума")

    coefficients = P372_ENVIRONMENTS[environment_id]
    in_domain = coefficients is None or 0.3 <= frequency <= 250.0
    if coefficients is None or not in_domain:
        fa_db = 0.0  # f_a = 1: опорный шум антенны при T0.
    else:
        c_value, d_value = coefficients
        fa_db = c_value - d_value * math.log10(frequency)

    try:
        fa_linear = 10.0 ** (fa_db / 10.0)
        receiver_linear = 10.0 ** (noise_figure / 10.0)
    except OverflowError as error:
        raise OracleError("NUMERIC_FAILURE", "коэффициент шума переполнен") from error
    system_factor = fa_linear + receiver_linear - 1.0
    if not math.isfinite(system_factor) or system_factor <= 0:
        raise OracleError("NUMERIC_FAILURE", "коэффициент шума системы недопустим")
    return {
        "faDb": fa_db,
        "noiseDbm": thermal_dbm + 10.0 * math.log10(system_factor),
        "noiseModelInDomain": in_domain,
    }


def stable_power_sum(powers_dbm: Any) -> float:
    if not isinstance(powers_dbm, list) or not powers_dbm:
        raise OracleError("INVALID_INTERFERENCE", "список мощностей должен быть непустым")
    powers = [
        require_finite(value, "INVALID_INTERFERENCE", "powersDbm") for value in powers_dbm
    ]
    maximum = max(powers)
    scaled_sum = math.fsum(10.0 ** ((value - maximum) / 10.0) for value in powers)
    result = maximum + 10.0 * math.log10(scaled_sum)
    if not math.isfinite(result):
        raise OracleError("NUMERIC_FAILURE", "сумма мощностей не конечна")
    return result


def linear_test_intelligibility(sinr_db: Any, params: Any) -> float:
    sinr = require_finite(sinr_db, "NUMERIC_FAILURE", "sinrDb")
    if not isinstance(params, dict):
        raise OracleError("CATALOG_VIOLATION", "параметры linear_test отсутствуют")
    low = require_finite(params.get("a"), "CATALOG_VIOLATION", "linear_test.a")
    high = require_finite(params.get("b"), "CATALOG_VIOLATION", "linear_test.b")
    if high <= low:
        raise OracleError("CATALOG_VIOLATION", "linear_test требует b > a")
    return min(1.0, max(0.0, (sinr - low) / (high - low)))


def invalid_budget(reason: str) -> dict[str, Any]:
    """Invalid budget не выпускает NaN/Inf наружу."""
    return {
        "valid": False,
        "reason": reason,
        "distanceM": 0.0,
        "layers": [],
        "totalLossDb": 0.0,
        "rxPowerDbm": 0.0,
        "thermalNoiseDbm": 0.0,
        "noiseDbm": 0.0,
        "noisePlusInterferenceDbm": 0.0,
        "sinrDb": 0.0,
        "sensitivityDbm": 0.0,
        "noiseModelInDomain": False,
        "intelligibility": 0.0,
        "state": "LOST",
    }


def validate_query(query: dict[str, Any]) -> None:
    distance_3d(query.get("txPos"), query.get("rxPos"))

    frequency = require_finite(
        query.get("frequencyMHz"), "INVALID_FREQUENCY", "frequencyMHz"
    )
    if frequency <= 0:
        raise OracleError("INVALID_FREQUENCY", "frequencyMHz должна быть положительной")

    bandwidth = require_finite(
        query.get("bandwidthHz"), "INVALID_BANDWIDTH", "bandwidthHz"
    )
    if bandwidth <= 0:
        raise OracleError("INVALID_BANDWIDTH", "bandwidthHz должна быть положительной")

    require_finite(query.get("txPowerDbm"), "CATALOG_VIOLATION", "txPowerDbm")
    for field in ("txGainDbi", "rxGainDbi"):
        require_finite(query.get(field), "INVALID_GAIN", field)

    noise_figure = require_finite(
        query.get("rxNoiseFigureDb"), "INVALID_NOISE_FIGURE", "rxNoiseFigureDb"
    )
    if noise_figure < 0:
        raise OracleError("INVALID_NOISE_FIGURE", "rxNoiseFigureDb не может быть отрицательным")

    has_sensitivity = require_bool(
        query.get("hasRxSensitivity"), "CATALOG_VIOLATION", "hasRxSensitivity"
    )
    has_required_sinr = require_bool(
        query.get("hasRequiredSinr"), "CATALOG_VIOLATION", "hasRequiredSinr"
    )
    if has_sensitivity == has_required_sinr:
        raise OracleError("CATALOG_VIOLATION", "нужно задать ровно один порог приёма")
    if has_sensitivity:
        require_finite(
            query.get("rxSensitivityDbm"), "INVALID_SENSITIVITY", "rxSensitivityDbm"
        )
    else:
        require_finite(
            query.get("requiredSinrDb"), "INVALID_REQUIRED_SINR", "requiredSinrDb"
        )

    has_interference = require_bool(
        query.get("hasInterference"), "CATALOG_VIOLATION", "hasInterference"
    )
    if has_interference:
        require_finite(
            query.get("interferenceDbm"), "INVALID_INTERFERENCE", "interferenceDbm"
        )

    if not isinstance(query.get("bandId"), str) or not query["bandId"]:
        raise OracleError("UNKNOWN_BAND", "bandId отсутствует")
    if query.get("modelId") != "free_space":
        raise OracleError("UNKNOWN_MODEL", "oracle v1 поддерживает только free_space")

    environment = query.get("noiseEnvironmentId")
    if not isinstance(environment, str) or environment not in P372_ENVIRONMENTS:
        raise OracleError("UNKNOWN_NOISE_ENVIRONMENT", "неизвестная категория внешнего шума")

    if query.get("intelligibilityModelId") != "linear_test":
        raise OracleError("UNKNOWN_MODEL", "oracle v1 допускает только TEST_ONLY linear_test")


def evaluate_budget(query: dict[str, Any], fixture: dict[str, Any]) -> dict[str, Any]:
    query = decode_special_floats(query)
    try:
        validate_query(query)
        distance = distance_3d(query["txPos"], query["rxPos"])
        fspl = free_space_loss(distance, query["frequencyMHz"])
        thermal_dbm = thermal_noise(query["bandwidthHz"])
        system = system_noise(
            query["bandwidthHz"],
            query["rxNoiseFigureDb"],
            query["noiseEnvironmentId"],
            query["frequencyMHz"],
        )
        total_loss = fspl["lossDb"]
        rx_power = (
            float(query["txPowerDbm"])
            + float(query["txGainDbi"])
            + float(query["rxGainDbi"])
            - total_loss
        )
        noise_dbm = float(system["noiseDbm"])
        if query.get("hasInterference") is True:
            noise_plus_interference = stable_power_sum(
                [noise_dbm, query["interferenceDbm"]]
            )
        else:
            noise_plus_interference = noise_dbm
        sinr = rx_power - noise_plus_interference

        if query.get("hasRxSensitivity") is True:
            sensitivity = float(query["rxSensitivityDbm"])
        else:
            sensitivity = (
                thermal_dbm
                + float(query["rxNoiseFigureDb"])
                + float(query["requiredSinrDb"])
            )

        if rx_power < sensitivity:
            intelligibility = 0.0
            state = "LOST"
        else:
            intelligibility = linear_test_intelligibility(sinr, fixture)
            q_clean = require_finite(fixture.get("qClean"), "CATALOG_VIOLATION", "qClean")
            q_degraded = require_finite(
                fixture.get("qDegraded"), "CATALOG_VIOLATION", "qDegraded"
            )
            if not 0.0 <= q_degraded <= q_clean <= 1.0:
                raise OracleError("CATALOG_VIOLATION", "пороги состояний недопустимы")
            if intelligibility >= q_clean:
                state = "CLEAN"
            elif intelligibility >= q_degraded:
                state = "DEGRADED"
            else:
                state = "WEAK"

        result = {
            "valid": True,
            "reason": "OK",
            "distanceM": distance,
            "layers": [{"layerId": "fspl", "lossDb": total_loss}],
            "totalLossDb": total_loss,
            "rxPowerDbm": rx_power,
            "thermalNoiseDbm": thermal_dbm,
            "noiseDbm": noise_dbm,
            "noisePlusInterferenceDbm": noise_plus_interference,
            "sinrDb": sinr,
            "sensitivityDbm": sensitivity,
            "noiseModelInDomain": bool(system["noiseModelInDomain"]),
            "intelligibility": intelligibility,
            "state": state,
        }
        if not all_numeric_values_finite(result):
            raise OracleError("NUMERIC_FAILURE", "budget содержит NaN/Inf")
        return result
    except OracleError as error:
        return invalid_budget(error.reason)
    except (OverflowError, ValueError):
        return invalid_budget("NUMERIC_FAILURE")


def all_numeric_values_finite(value: Any) -> bool:
    if isinstance(value, bool) or value is None or isinstance(value, str):
        return True
    if isinstance(value, (int, float)):
        return math.isfinite(float(value))
    if isinstance(value, list):
        return all(all_numeric_values_finite(item) for item in value)
    if isinstance(value, dict):
        return all(all_numeric_values_finite(item) for item in value.values())
    return False


def evaluate_vector(vector: dict[str, Any], document: dict[str, Any]) -> dict[str, Any]:
    kind = vector.get("kind")
    data = decode_special_floats(vector.get("input", {}))
    if kind == "fspl":
        return free_space_loss(data.get("distanceM"), data.get("frequencyMHz"))
    if kind == "thermal_noise":
        return {"thermalNoiseDbm": thermal_noise(data.get("bandwidthHz"))}
    if kind == "system_noise":
        return system_noise(
            data.get("bandwidthHz"),
            data.get("rxNoiseFigureDb"),
            data.get("noiseEnvironmentId"),
            data.get("frequencyMHz"),
        )
    if kind == "power_sum":
        return {"sumDbm": stable_power_sum(data.get("powersDbm"))}
    if kind == "intelligibility_linear_test":
        return {
            "intelligibility": linear_test_intelligibility(
                data.get("sinrDb"), data.get("params")
            )
        }
    if kind in {"budget", "invalid"}:
        fixture = document.get("fixtures", {}).get("linear_test", {})
        result = evaluate_budget(data, fixture)
        if kind == "invalid":
            result["allFinite"] = all_numeric_values_finite(result)
        return result
    raise OracleError("SCHEMA_ERROR", f"неизвестный kind: {kind!r}")


def tolerance_for(key: str, tolerances: dict[str, Any]) -> float:
    lowered = key.lower()
    if key.endswith(("Dbm", "Db", "Dbi")):
        return float(tolerances["db"])
    if key.endswith("M") or "distance" in lowered:
        return float(tolerances["m"])
    if "intelligibility" in lowered or key.startswith(("q", "Q")):
        return float(tolerances["ratio"])
    return float(tolerances["db"])


def compare_expected(
    expected: Any,
    actual: Any,
    tolerances: dict[str, Any],
    path: str = "expected",
) -> list[str]:
    errors: list[str] = []
    if isinstance(expected, bool):
        if actual is not expected:
            errors.append(f"{path}: ожидалось {expected}, получено {actual!r}")
        return errors
    if isinstance(expected, (int, float)):
        if isinstance(actual, bool) or not isinstance(actual, (int, float)):
            return [f"{path}: ожидалось число, получено {actual!r}"]
        tolerance = tolerance_for(path.rsplit(".", 1)[-1], tolerances)
        if not math.isfinite(float(actual)) or abs(float(actual) - float(expected)) > tolerance:
            errors.append(
                f"{path}: ожидалось {expected} ± {tolerance}, получено {actual}"
            )
        return errors
    if isinstance(expected, str):
        if actual != expected:
            errors.append(f"{path}: ожидалось {expected!r}, получено {actual!r}")
        return errors
    if isinstance(expected, list):
        if not isinstance(actual, list) or len(actual) != len(expected):
            return [f"{path}: длина/тип списка не совпадает"]
        for index, item in enumerate(expected):
            errors.extend(compare_expected(item, actual[index], tolerances, f"{path}[{index}]"))
        return errors
    if isinstance(expected, dict):
        if not isinstance(actual, dict):
            return [f"{path}: ожидался объект, получено {actual!r}"]
        for key, item in expected.items():
            if key not in actual:
                errors.append(f"{path}.{key}: поле отсутствует")
                continue
            errors.extend(compare_expected(item, actual[key], tolerances, f"{path}.{key}"))
        return errors
    if actual != expected:
        errors.append(f"{path}: ожидалось {expected!r}, получено {actual!r}")
    return errors


def load_vector_document(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as stream:
        document = json.load(stream)
    if document.get("schema") != "ricon.oracle.link_eval" or document.get("version") != 1:
        raise OracleError("SCHEMA_ERROR", "неподдерживаемая schema/version")
    vectors = document.get("vectors")
    if not isinstance(vectors, list) or not vectors:
        raise OracleError("SCHEMA_ERROR", "vectors должен быть непустым списком")
    identifiers = [vector.get("id") for vector in vectors]
    if any(not isinstance(identifier, str) or not identifier for identifier in identifiers):
        raise OracleError("SCHEMA_ERROR", "каждый vector должен иметь id")
    if len(identifiers) != len(set(identifiers)):
        raise OracleError("SCHEMA_ERROR", "vector id должны быть уникальны")
    return document


def check_document(document: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    tolerances = document.get("tolerances", {})
    for name in ("db", "ratio", "m"):
        value = tolerances.get(name)
        if isinstance(value, bool) or not isinstance(value, (int, float)) or value <= 0:
            failures.append(f"schema: tolerance {name} недопустим")
    if failures:
        return failures

    for vector in document["vectors"]:
        identifier = vector["id"]
        try:
            actual = evaluate_vector(vector, document)
            failures.extend(
                f"{identifier}: {failure}"
                for failure in compare_expected(
                    vector.get("expected", {}), actual, tolerances
                )
            )
        except (KeyError, TypeError, ValueError, OverflowError, OracleError) as error:
            failures.append(f"{identifier}: исключение {type(error).__name__}: {error}")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path, metavar="VECTORS_JSON")
    args = parser.parse_args(argv)
    if args.check is None:
        parser.error("нужен --check VECTORS_JSON")
    try:
        document = load_vector_document(args.check)
        failures = check_document(document)
    except (OSError, json.JSONDecodeError, OracleError) as error:
        print(f"ORACLE_FAIL: {error}", file=sys.stderr)
        return 1
    if failures:
        for failure in failures:
            print(f"ORACLE_FAIL: {failure}", file=sys.stderr)
        return 1
    print(f"ORACLE_PASS: {len(document['vectors'])} vectors")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
