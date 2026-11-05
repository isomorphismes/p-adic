# Charcoal presentation

Shared presentation owner: android-NDK `e8b67dfc67776c2b8da919785d15f38d68c91db0`.
The four resource files are byte-checked against that owner.
The application explicitly uses its fullscreen charcoal theme, independent of
system night mode. Startup windows, framework dialogs and system bars use the
same fixed palette.

Browser panels, controls and labels use the same neutral palette; the neutral GLES canvas and unselected ring surfaces become #212121/#2F2F2F. Path, selected-digit, prime and ghost colors retain their mathematical meaning. The JavaScript arithmetic, SVG geometry, touch interpretation and ring state are unchanged.

`android/link-resources.grease` compiles and links the resource table at the
existing manifest/API boundary. It is invoked by the existing packager through
WITT_GREASE, the absolute canonical Grease entrypoint. The short helper
uses Grease because it sequences Android packaging programs; it adds no
application logic or another source generator. Its parse check is separate
from actual aapt2 execution in CI. No Bash business logic is added.

The existing public test signer, package and version are retained. The maintained builder explicitly supports ARMv7 only; this is not an ARM64/C67 delivery. A new ABI or emulator port is not silently inferred from a successful ARMv7 package.

API29/API34 appearance/state execution and physical C67 acceptance remain
NOT_RUN. Existing host/native compilation and palette-copy checks cannot
establish those results. No installed app or public deployment is changed.
