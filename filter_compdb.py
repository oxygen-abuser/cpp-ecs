import json

with open('build/compile_commands.json') as f:
    db = json.load(f)

filtered = [e for e in db if not (e['file'].endswith('.c') and '_deps' in e['file'])]
removed = [e for e in db if e not in filtered]

print(f"removed {len(removed)} entries, {len(filtered)} remaining")

with open('build/compile_commands.json', 'w') as f:
    json.dump(filtered, f, indent=2)
