with open('out') as f:
    results = []

    command = None
    for line in f:
        if line.startswith('Command:'):
            command = line
        if line.startswith('stats:'):
            results.append((command, line))

    results = sorted(results, key=lambda r: float(r[1].strip().split(', ')[-1]), reverse=True)
    for r in results[:5]: print(r)
