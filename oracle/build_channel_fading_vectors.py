#!/usr/bin/env python3
"""P1b.1 DRAFT: тестовые входы и корпус; ожидания из авторского эталона, freeze ещё нет."""
import argparse
import copy
import hashlib
import json
from pathlib import Path

from channel_fading_oracle import M, QUARTER, baseline, evaluate


def component(cid, weight=1, phase=0, doppler=0):
    return dict(componentId=cid, powerWeight=weight, phaseQ=phase, dopplerQ=doppler)


def burst_input():
    data = baseline()
    data["descriptor"].update(anchorMs=0, validFromMs=0, validUntilMs=31000, staticChannel=False)
    phases = (0, 123456789, 345678901, 876543210, 112233445, 998877665)
    rates = (0, 3211, 7571, 11557, 19001, 33113)
    data["descriptor"]["components"] = [component(f"path{i}", phase=p, doppler=r) for i, (p, r) in enumerate(zip(phases, rates))]
    data["query"]["nowMs"] = 0
    return data


def build():
    cases, host_only = [], []

    def add(title, edit=lambda x: None, source=None, host=False):
        data = copy.deepcopy(baseline() if source is None else source)
        edit(data)
        cid = f"F{len(cases) + 1:02}"
        cases.append(dict(id=cid, title=title, input=data, expect=evaluate(data)))
        if host:
            host_only.append(cid)

    add("Одна составляющая, единичный коэффициент среды")
    add("Точная четверть оборота", lambda x: x["descriptor"]["components"][0].update(phaseQ=QUARTER))
    add("Конструктивная интерференция двух путей", lambda x: x["descriptor"].update(components=[component("a"), component("b")]))
    opposite = baseline()
    opposite["descriptor"].update(staticChannel=False, components=[component("a"), component("b", phase=M // 2, doppler=1074)])
    add("Мгновенная компенсация, descriptor не удаляется", source=opposite)
    add("Тот же канал позже восстанавливается", lambda x: x["query"].update(nowMs=1250), opposite)
    add("Отрицательный доплер и перенос фазы", lambda x: (x["descriptor"].update(staticChannel=False),
        x["descriptor"]["components"][0].update(dopplerQ=-1074), x["query"].update(nowMs=1250)))
    add("Малый остаток рядом с противофазой", lambda x: x["descriptor"].update(components=[component("a"), component("b", phase=M // 2 + 1)]))
    add("Слабый путь после компенсации сильных", lambda x: x["descriptor"].update(components=[
        component("a"), component("b", phase=M // 2), component("c", weight=1e-24)]))
    add("Максимальное время без float-произведения", lambda x: (x["descriptor"].update(anchorMs=0, validFromMs=0,
        validUntilMs=2147483647, staticChannel=False), x["descriptor"]["components"][0].update(dopplerQ=-QUARTER + 3),
        x["query"].update(nowMs=2147483646, subUs=999)))
    add("Микросекунды и верхняя граница доплера", lambda x: (x["descriptor"].update(staticChannel=False),
        x["descriptor"]["components"][0].update(dopplerQ=QUARTER), x["query"].update(subUs=999)))
    add("Ненулевой доплер у статичной среды недопустим", lambda x: x["descriptor"]["components"][0].update(dopplerQ=1))
    add("Исчерпан ресурс", lambda x: x["descriptor"].update(resourceExhausted=True))
    add("Неполное физическое описание", lambda x: x["descriptor"].update(coverageComplete=False))
    add("Смена условий среды", lambda x: x["query"].update(environmentRevision=2))
    add("Другая частота", lambda x: x["query"].update(frequencyMHz=151))
    add("Другая версия профиля", lambda x: x["query"].update(profileVersion=2))
    add("Неподдерживаемая модель", lambda x: x["descriptor"].update(modelId="OTHER_MODEL"))
    add("Пустое описание полезной среды", lambda x: x["descriptor"].update(components=[]))
    add("Все веса нулевые", lambda x: x["descriptor"]["components"][0].update(powerWeight=0))
    add("Последняя запись невалидна, несмотря на ресурсный отказ", lambda x: (x["descriptor"].update(resourceExhausted=True),
        x["descriptor"]["components"].append(component("z", weight=-1))))
    add("До начала интервала", lambda x: x["query"].update(nowMs=999))
    add("Правая граница интервала исключена", lambda x: x["query"].update(nowMs=5000))
    add("Доплер вне области модели", lambda x: (x["descriptor"].update(staticChannel=False),
        x["descriptor"]["components"][0].update(dopplerQ=QUARTER + 1)))
    add("INT_MIN доплера отвергается до abs", lambda x: (x["descriptor"].update(staticChannel=False),
        x["descriptor"]["components"][0].update(dopplerQ=-2147483648)))
    add("Дубликат componentId", lambda x: x["descriptor"]["components"].append(component("direct")))
    add("Нарушен порядок componentId", lambda x: x["descriptor"].update(components=[component("b"), component("a")]))
    add("bool вместо времени", lambda x: x["query"].update(nowMs=True), host=True)
    add("bool вместо веса", lambda x: x["descriptor"]["components"][0].update(powerWeight=True), host=True)
    add("Лишнее поле JSON", lambda x: x["descriptor"].update(extra=1), host=True)
    add("Пропущена версия профиля JSON", lambda x: x["descriptor"].pop("profileVersion"), host=True)
    add("Переполнение binary32 веса", lambda x: x["descriptor"]["components"][0].update(powerWeight=1e39))
    add("Недопустимый символ ключа", lambda x: x["descriptor"]["components"][0].update(componentId="bad/key"))
    add("Фаза за пределом полного оборота", lambda x: x["descriptor"]["components"][0].update(phaseQ=M))
    add("Минимальный положительный binary32 вес", lambda x: x["descriptor"]["components"][0].update(powerWeight=2**-149))
    add("Взаимный канал при обратном направлении", lambda x: x["query"].update(tx=x["descriptor"]["b"], rx=x["descriptor"]["a"]), opposite)
    add("Descriptor неканоничен", lambda x: x["descriptor"].update(a=x["query"]["rx"], b=x["query"]["tx"]))
    add("Одинаковая физическая антенна с обеих сторон", lambda x: x["query"].update(rx=x["query"]["tx"]))
    add("Неверная микросекундная доля", lambda x: x["query"].update(subUs=1000))
    add("Частоты совпадают после binary32", lambda x: x["descriptor"].update(frequencyMHz=150.000001))
    for time in (0, 380, 390, 400, 410, 670, 680, 690, 700, 1000, 1230):
        add(f"TEST_ONLY неравномерное окно, t={time} ms", lambda x, t=time: x["query"].update(nowMs=t), burst_input())
    return dict(schema="ricon-channel-fading-v1", version=1, requirement="REQ-channel-fading v0.1 DRAFT",
                note="TEST_ONLY. Не профиль рации, не доказательство доставки или точного Rayleigh/Rice finite-N.",
                hostOnly=host_only, cases=cases)


def canonical_text():
    return json.dumps(build(), ensure_ascii=False, indent=2, allow_nan=False) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    target = Path(__file__).resolve().parent / "vectors/channel_fading_v1.json"
    text = canonical_text()
    if args.check:
        if target.read_bytes() != text.encode("utf-8"):
            raise SystemExit("CHANNEL_FADING_VECTORS_FAIL: корпус отличается")
    else:
        target.write_text(text, encoding="utf-8", newline="\n")
    corpus = build()
    print(f"CHANNEL_FADING_VECTORS_PASS: {len(corpus['cases'])}, host-only={corpus['hostOnly']}, SHA={hashlib.sha256(text.encode('utf-8')).hexdigest()}")


if __name__ == "__main__":
    main()
