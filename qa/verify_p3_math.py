import math


def consensus(current, h1, h2, detail_guard=0.0, tail_guard=0.0):
    minimum = min(current, h1, h2)
    maximum = max(current, h1, h2)
    median = current + h1 + h2 - minimum - maximum
    lifted = max(current, median)
    wanted_guard = max(0.0, min(1.0, max(detail_guard, 0.65 * tail_guard)))
    strength = 0.86 * (1.0 - wanted_guard)
    return current + strength * (lifted - current)


def tail_decay(value, elapsed_seconds, tau=0.240):
    return value * math.exp(-elapsed_seconds / tau)


checks = []

def check(name, condition, detail=""):
    checks.append((name, bool(condition), detail))


hole = consensus(0.20, 0.80, 0.82)
check("isolated deep hole is lifted", hole > 0.68, f"{hole:.3f}")

steady = consensus(0.28, 0.29, 0.27)
check("sustained reduction remains reduced", steady < 0.31, f"{steady:.3f}")

attack = consensus(0.20, 0.80, 0.82, detail_guard=1.0)
check("detail guard disables consensus smear", abs(attack - 0.20) < 1e-6, f"{attack:.3f}")

partial_attack = consensus(0.20, 0.80, 0.82, detail_guard=0.75)
check("partial transient protection reduces consensus lift", partial_attack < hole - 0.20, f"{partial_attack:.3f}")

tail_guarded = consensus(0.20, 0.80, 0.82, tail_guard=0.90)
check("tail memory relaxes consensus", tail_guarded < hole, f"{tail_guarded:.3f}")

at_100ms = tail_decay(1.0, 0.100)
at_240ms = tail_decay(1.0, 0.240)
at_500ms = tail_decay(1.0, 0.500)
check("tail memory remains useful at 100 ms", 0.60 < at_100ms < 0.70, f"{at_100ms:.3f}")
check("tail time constant is 240 ms", 0.36 < at_240ms < 0.38, f"{at_240ms:.3f}")
check("tail memory decays rather than latching", at_500ms < 0.13, f"{at_500ms:.3f}")

failed = [name for name, ok, _ in checks if not ok]
print("SMART DENOISE P3 BEHAVIOR MODEL")
print("===============================")
for name, ok, detail in checks:
    suffix = f" ({detail})" if detail else ""
    print(f"[{'PASS' if ok else 'FAIL'}] {name}{suffix}")
print(f"Checks: {len(checks)}  Passed: {len(checks)-len(failed)}  Failed: {len(failed)}")
raise SystemExit(1 if failed else 0)
