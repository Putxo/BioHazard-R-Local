"""Two indexed PadData flag queries reached from per-slot action producers."""
SITES=(0x02DAE728,0x02DAE898)
def patches():
    for at in SITES:
        yield at,bytes.fromhex('8b8870090000'),bytes.fromhex('8b4d08909090'),'pad_state_'+hex(at)
