"""January raw axis producers: preserve the explicit logical-pad argument."""
# Both stick pairs, then the positive/negative D-pad fallback for each axis.
SITES = (
    (0x02DB1205, 'edx', 'stick_a_x'),
    (0x02DB1295, 'edx', 'stick_a_y'),
    (0x02DB1325, 'edx', 'stick_b_x'),
    (0x02DB13B5, 'edx', 'stick_b_y'),
    (0x02DB143D, 'ecx', 'dpad_x_positive'),
    (0x02DB1461, 'ecx', 'dpad_x_negative'),
    (0x02DB14ED, 'ecx', 'dpad_y_positive'),
    (0x02DB1511, 'ecx', 'dpad_y_negative'),
)

def patches():
    for va, register, label in SITES:
        old = bytes.fromhex('8b9170090000' if register == 'edx' else '8b8870090000')
        new = bytes.fromhex('8b5508909090' if register == 'edx' else '8b4d08909090')
        yield va, old, new, 'pad_axis_' + label

def inspect_axes(pe):
    states = []
    for va, old, new, label in patches():
        value = pe.read(va, len(old))
        states.append('ARGUMENT' if value == new else 'GLOBAL' if value == old else 'UNKNOWN')
    status = ('PAD_ARGUMENTS_PRESENT' if all(s == 'ARGUMENT' for s in states) else
              'LEGACY_GLOBAL_AXES' if all(s == 'GLOBAL' for s in states) else 'MIXED_OR_UNKNOWN_AXES')
    return {'status': status, 'argument_sites': states.count('ARGUMENT'),
            'sites': len(states), 'gameplay_validated': False}
