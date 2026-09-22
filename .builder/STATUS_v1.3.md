# CrossPoint MAX v1.3-dev

Implemented:
- Page translation.
- Study Mode page flow.
- Chapter translation with checkpoint/resume.
- Whole-book translation over all EPUB spine entries.
- Missing section cache is incrementally built (`buildSomeMore(3)` per activity tick).
- Page translation runs one page per tick.
- Whole-book checkpoint stores next spine/page and survives cancellation/error.
- Translation/provider error leaves the failed page as the resume point.
- Cache v2 includes a fingerprint of the rendered source-page text.

Safety:
- Whole-book translation refuses to run when cache is disabled.
- Invalid/unknown viewport blocks the job.
- No erase/flash commands.
- No merged/full-flash image.
- Existing CI still verifies source flash size, application offset, ESP image,
  and application range before publishing the BIN.

Not implemented:
- MAX Library / series / collections.
- Final MAX Home.
- Physical-device verification of this v1.3 build.
