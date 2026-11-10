"""Static envelope arithmetic and accepted ASM pins; not native execution/world coverage."""
from pathlib import Path
import hashlib
import json
import math
import struct

ROOT = Path(__file__).resolve().parent
TMP = Path(r'C:/Users/Ded/AppData/Local/Temp/ricon-von-re-20261104')
PIN = '1d221a990fa16c78fe02f685a7ba1330be115ca93cd3b641e67e5d3f1b40d9c9'
checks = []


def check(value, name):
    assert value, name
    checks.append(name)


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def native_axis(d, radius):
    d2 = f32(f32(d) * f32(d))
    r2 = f32(f32(radius) * f32(radius))
    return d2 <= r2, f32(1.0 - f32(d2 / r2))


def cells(x, radius):
    return math.floor((x + radius) / 50.0) - math.floor((x - radius) / 50.0) + 1


check(max(1, min(65535, 70000)) == 65535, 'finite_setter_upper_clamp')
check(max(1, min(65535, -1)) == 1, 'finite_setter_lower_clamp')
check(native_axis(65535, 65535) == (True, 0.0), 'native_float_axis_equal_boundary_admitted_q0')
check(not native_axis(65536, 65535)[0], 'axis_above_Rmax_rejected')
check(not native_axis(70000, 65535)[0], 'no_arbitrary_world_coverage')
check(native_axis(10000, 65535)[0] and native_axis(60000, 65535)[0], 'admitted_not_necessarily_near_one_quality')
q10 = 1 - (10000 / 65535) ** 2
q60 = 1 - (60000 / 65535) ** 2
check(0.9767 < q10 < 0.9768, 'q_lower_bound_example_10km')
check(0.1617 < q60 < 0.1619, 'q_lower_bound_example_60km')
check(math.isclose(65535 * math.sqrt(.01), 6553.5), 'q99_requires_D_at_most_R_times_point1')
check(2072 < 65535 * math.sqrt(.001) < 2073, 'q999_diameter_limit_example')
check(math.isclose(max(2, .05 * 65535), 3276.75), 'large_R_movement_threshold_not_update_period')
check(cells(0, 65535) == 2622 and cells(25, 65535) == 2623, '50m_broadphase_axis_counts')
check(2622 ** 2 == 6874884 and 2623 ** 2 == 6880129, 'broadphase_work_count_not_benchmark')
check(cells(0, 1300) ** 2 == 2809, 'small_R_reference_work_count')
check(math.sqrt(1000**2 + 2000**2 + 500**2) < 3000, 'illustrative_box_bound_not_actual_map')
check(native_axis(65520, 65535)[0] and not native_axis(65520, 65504)[0], 'effective_server_R_not_replica_half_readback')
check(struct.unpack('<e', struct.pack('<H', 0x7bff))[0] == 65504, 'half_range_replica_value')

paths = ['batch02/export/Radio_SetRange.asm', 'batch02/export/Radio_ConcreteSlot6.asm',
         'batch02/export/Radio_GraphCandidate.asm', 'batch02/export/VoN_RadioGraphCaller.asm',
         'batch05/export/World_CellEnumerator.asm', 'batch03/export/next_body_17.asm',
         'batch08/export/Radio_Serialize.asm']
files = []
for rel in paths:
    path = TMP / rel
    data = path.read_bytes()
    check(PIN in data.decode('utf-8'), 'accepted_pin_' + path.stem)
    files.append(dict(path=str(path), sha256=hashlib.sha256(data).hexdigest()))
receipt = dict(scope='STATIC_ARITHMETIC_AND_ACCEPTED_ASM_REUSE', checks=checks, passed=len(checks),
               gamePinReused=PIN, files=files, examples=dict(q10km=q10, q60km=q60),
               actualWorldBound='UNKNOWN_NOT_MEASURED', actualWorldCoverage='UNQUALIFIED',
               nativeEffect='NOT_RUN', audio='NOT_RUN', clientDSP='NOT_QUALIFIED',
               newCalleeRequests=0, modelNote='Arithmetic examples are not runtime/world evidence',
               reportSha256=hashlib.sha256((ROOT / 'REPORT_RU.md').read_bytes()).hexdigest())
(ROOT / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
print(json.dumps(dict(passed=len(checks), worldCoverage=receipt['actualWorldCoverage'],
                     reportSha256=receipt['reportSha256'], native='NOT_RUN')))
