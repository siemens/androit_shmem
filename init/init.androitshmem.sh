#!/system/bin/sh

/system/bin/AndroitShmemServer &
echo -17 > /proc/$!/oom_adj
