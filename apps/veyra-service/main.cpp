// Veyra Audio Service — Phase 0 no-op stub.
//
// Final form (Phase 1+): a Windows service running as LocalSystem that
// mediates between the UI and the APO, owns canonical config/preset state,
// runs game detection and the Sound Tracker engine, and hosts the named-pipe
// control + tracker servers. For Phase 0 it simply starts and exits cleanly.

int main()
{
    return 0;
}
