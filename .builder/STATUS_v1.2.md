# CrossPoint MAX v1.2-dev — status

Implemented:
- page translation and Study Mode from v1.1,
- chapter translation using the already-paginated current Section,
- one page per activity loop,
- cache reuse,
- JSON checkpoint after every completed/skipped page,
- BACK cancellation preserving checkpoint,
- resume for the same book/spine/target,
- fail-closed behavior while chapter pagination is incomplete.

Not implemented:
- whole-book/spine-to-spine worker,
- book-wide resume,
- MAX Library / series / collections,
- final physical X4 verification.

Safety:
- no erase command,
- no flash command,
- no merged/full-flash image,
- build pipeline still validates flash size, application offset, ESP image and range.
