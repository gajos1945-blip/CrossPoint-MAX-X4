# CrossPoint MAX v1.4-dev

Implemented:
- v1.1 page translator + Study Mode.
- v1.2 chapter translator + checkpoint/resume.
- v1.3 whole-book translator + content/layout-bound cache.
- v1.4 MAX Library and Home integration.

MAX Library design:
- SD-first line-oriented JSON index (`index.jsonl`) rather than one giant JSON
  document, so parsing is record-by-record.
- Max 600 indexed supported books in this development stage.
- Rebuild scans `/Books` when present, otherwise `/`.
- `.crosspoint` and `.crosspoint-max` private directories are never indexed.
- Cached EPUB metadata is reused without forcing expensive metadata creation.
- Missing title falls back to the filename.
- Collections/series use an explicit folder convention and are not guessed.
- Opening an EPUB marks it InProgress without downgrading a Read book.
- End-of-book marks it Read once.
- User can toggle Favorite and cycle status manually.

Home:
- Stock recent-cover/continue-reading behavior stays intact.
- MAX Library is a first-class Home menu item.
- Empty/non-title header uses "CrossPoint MAX".

Not implemented in v1.4:
- free-text on-device search,
- on-device series/collection text editing,
- EPUB series-number parsing,
- physical X4 verification.
