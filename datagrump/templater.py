import sys

template_file = sys.argv[1]
output_file = sys.argv[2]

## This only works for single letter
## variables, for ease of scripting.
variables_map = {}
for arg in sys.argv[3:]:
  var, name, val = arg.split(":")
  variables_map[var] = (val, name)

template_char = '^';

state = 0 # 0 if current character is regular, 1 if should be replaced.
with open(template_file) as f:
  with open(output_file, 'w') as f_out:
    for line in f:
        for c in line:
          if state == 1:
            f_out.write(variables_map[c][0])
            state = 0
          else:
            if c == template_char:
              state = 1
              continue
            else:
              f_out.write(c)

print("Command: " + " ".join(sys.argv))
