#!/usr/bin/env python3
"""Авторский корпус CT10.1: ожидаемые результаты из эталона, TEST_ONLY, не независимая приёмка."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct

import channel_descriptor_oracle as cd
import channel_fading_oracle as cf


def dynamic():
    d = cf.baseline()["descriptor"]
    d.update(staticChannel=False, validFromMs=1200, components=[
        dict(componentId="a", powerWeight=1.0, phaseQ=123, dopplerQ=11117),
        dict(componentId="b", powerWeight=.25, phaseQ=cf.QUARTER, dopplerQ=-12345)])
    return d


def build():
    cases, host = [], []

    def add(title, inp, host_only=False):
        cid = f"D{len(cases) + 1:02}"
        cases.append(dict(id=cid, title=title, input=copy.deepcopy(inp), expect=cd.evaluate(inp)))
        if host_only:
            host.append(cid)

    def enc(title, modify=None, budget=5120, host_only=False):
        d = cf.baseline()["descriptor"]
        if modify:
            modify(d)
        add(title, dict(op="ENCODE", descriptor=d, maxBytes=budget), host_only)

    def dec(title, modify=None, budget=5120, values=None, fix_length=False, host_only=False):
        values = list(cd.frame(cd.normal(cf.baseline()["descriptor"]))) if values is None else list(values)
        if modify:
            modify(values)
        if fix_length:
            values[5:7] = list(struct.pack(">H", len(values)))
        add(title, dict(op="DECODE", bytes=values, maxBytes=budget), host_only)

    enc("Один статичный луч, точная упаковка")
    dec("Распаковка одного луча")
    enc("Пустой набор сохраняется", lambda d: d.update(components=[]))
    enc("Нулевой вес не запрещает передачу описания", lambda d: d["components"][0].update(powerWeight=0))
    enc("Неполное покрытие сохраняется флагом", lambda d: d.update(coverageComplete=False))
    enc("Исчерпанный ресурс сохраняется флагом", lambda d: d.update(resourceExhausted=True))
    enc("Наименьший положительный вес не отсекается", lambda d: d["components"][0].update(powerWeight=2**-149))
    enc("FLT_MAX и минимальная полоса сохраняются", lambda d: d.update(frequencyMHz=cf.f32(3.4028234663852886e38), bandwidthHz=2**-149))
    enc("Отрицательный ноль канонизируется к +0", lambda d: d["components"][0].update(powerWeight=-0.0))
    enc("Неверная каноническая пара", lambda d: d.update(a=d["b"], b=d["a"]))
    enc("Неподдерживаемая модель", lambda d: d.update(modelId="OTHER_MODEL"))
    enc("65 составляющих не обрезаются", lambda d: d.update(components=[dict(componentId=f"c{i:02}", powerWeight=1, phaseQ=0, dopplerQ=0) for i in range(65)]))
    maximum = cf.baseline()["descriptor"]
    maximum.update(profileId="p" * 64, components=[dict(componentId=f"c{i:02}" + "x" * 61, powerWeight=1, phaseQ=i, dopplerQ=0) for i in range(64)])
    add("Предельный поддерживаемый кадр ровно в бюджете", dict(op="ENCODE", descriptor=maximum, maxBytes=5066))
    add("Один байт сверх бюджета — отказ без усечения", dict(op="ENCODE", descriptor=maximum, maxBytes=5065))
    enc("Нулевой бюджет недопустим", budget=0)
    enc("Бюджет выше uint16 недопустим", budget=65536)
    enc("Bool не бюджет", budget=True, host_only=True)
    enc("Дробная форма бюджета не int", budget=102.0, host_only=True)
    enc("Bool не вес", lambda d: d["components"][0].update(powerWeight=True), host_only=True)
    enc("Лишний epoch не становится физической идентичностью", lambda d: d.update(epoch=7), host_only=True)
    enc("Пропущенное поле дескриптора", lambda d: d.pop("profileId"), host_only=True)
    enc("Статичный канал с доплером неверен", lambda d: d["components"][0].update(dopplerQ=1))
    enc("Неверный порядок составляющих", lambda d: d.update(components=[dict(componentId=k, powerWeight=1, phaseQ=0, dopplerQ=0) for k in ("b", "a")]))
    dec("Неверный magic", lambda b: b.__setitem__(0, 0))
    dec("Неизвестная версия целого кадра", lambda b: b.__setitem__(4, 2))
    dec("Потерян последний байт", lambda b: b.pop())
    dec("Неполное тело при правильной внешней длине", lambda b: b.pop(), fix_length=True)
    dec("Лишний хвост при правильной внешней длине", lambda b: b.append(0), fix_length=True)
    dec("Размер больше бюджета", budget=101)
    dec("Byte вне 0..255", lambda b: b.__setitem__(8, 256))
    dec("Bool не byte", lambda b: b.__setitem__(8, True), host_only=True)
    dec("Нулевая длина ключа", lambda b: b.__setitem__(7, 0))
    dec("Не ASCII", lambda b: b.__setitem__(8, 255))
    dec("Зарезервированный бит flags", lambda b: b.__setitem__(81, b[81] | 8))
    dec("Счётчик 65 до выделения компонентов", lambda b: b.__setitem__(82, 65))
    dec("NaN веса отвергается", lambda b: b.__setitem__(slice(90, 94), list(bytes.fromhex("7fc00000"))))
    dec("Infinity частоты отвергается", lambda b: b.__setitem__(slice(57, 61), list(bytes.fromhex("7f800000"))))
    dec("Неканонический отрицательный ноль отвергается", lambda b: b.__setitem__(slice(90, 94), list(bytes.fromhex("80000000"))))
    dec("Корректный ключ другой модели", lambda b: b.__setitem__(8, ord("X")))
    out_domain = dynamic()
    out_domain["components"][0]["dopplerQ"] = -(1 << 31)
    add("Кодек сохраняет OUT_OF_MODEL_DOMAIN, не делает LOST", dict(op="ENCODE", descriptor=out_domain, maxBytes=5120))
    add("Decode сохраняет полный int32 доплера", dict(op="DECODE", bytes=list(cd.frame(cd.normal(out_domain))), maxBytes=5120))
    for anchor in (1200, 1234, 4999, 1199, 5000):
        add(f"Перенос якоря динамического канала: {anchor}", dict(op="REBASE", descriptor=dynamic(), anchorMs=anchor))
    add("Bool не якорь", dict(op="REBASE", descriptor=dynamic(), anchorMs=True), host_only=True)
    add("Физический домен проверяется до якоря", dict(op="REBASE", descriptor=out_domain, anchorMs=-1))
    large = dynamic()
    large.update(anchorMs=0, validFromMs=0, validUntilMs=cf.MAX)
    add("Большое время не переводится во float", dict(op="REBASE", descriptor=large, anchorMs=cf.MAX-1))
    add("Статичный процесс при новом якоре", dict(op="REBASE", descriptor=cf.baseline()["descriptor"], anchorMs=4000))
    add("Null descriptor", dict(op="ENCODE", descriptor=None, maxBytes=5120))
    add("Null frame", dict(op="DECODE", bytes=None, maxBytes=5120))
    dec("Слишком короткий заголовок", values=[82, 67])
    add("Ресурсный отказ до обхода элементов", dict(op="DECODE", bytes=[True, 999], maxBytes=1), host_only=True)
    return dict(schema="ricon-channel-descriptor-v1", version=1, requirement="REQ-channel-descriptor v0.1 AGENT-REVIEWED",
                note="TEST_ONLY. RCFD reference bytes, не Rpl/RPC/clock/audio acceptance.",
                # Ручная разметка CD07, не результат Encode: исходный CF baseline, 102 байта.
                goldenHex="524346440100661346494e4954455f4d554c5449504154485f563109544553545f4f4e4c59"
                          "000000010000000a0000000000000014000000004316000046c3500000000001000003e8"
                          "000003e8000013880301066469726563743f8000000000000000000000",
                hostOnly=host, cases=cases)


def canonical():
    d = build()
    header = {key: value for key, value in d.items() if key != "cases"}
    prefix = json.dumps(header, ensure_ascii=False, indent=2)[:-2]
    return prefix + ',\n  "cases": [\n' + ',\n'.join('    ' + json.dumps(c, ensure_ascii=False, allow_nan=False) for c in d["cases"]) + '\n  ]\n}\n'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--check", action="store_true")
    args = p.parse_args()
    target = Path(__file__).with_name("vectors") / "channel_descriptor_v1.json"
    raw = canonical().encode("utf-8")
    if args.check:
        if target.read_bytes() != raw:
            raise SystemExit("CHANNEL_DESCRIPTOR_VECTORS_FAIL")
    else:
        target.write_bytes(raw)
    print(f"CHANNEL_DESCRIPTOR_VECTORS_PASS: {len(build()['cases'])}, SHA={hashlib.sha256(raw).hexdigest()}")


if __name__ == "__main__":
    main()
