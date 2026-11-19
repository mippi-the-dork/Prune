# Maintaining Prune Documentation

## Source and quality

This manual documents **Prune v1.0.0** using the plugin's public C++ implementation, descriptor and README. Check current source before describing new behavior. Do not present an unimplemented roadmap proposal as a shipped feature.

The native Actor Details workflow is the source of truth: keep compatible Unreal selection and Property Section behavior, and distinguish user-configured filter visibility from actual Actor data.

## Design vs Technical

Each feature topic has two distinct pages:
- **Design:** what the feature does, when it is useful, complete user steps, troubleshooting, concrete examples and limits.
- **Technical:** C++ owner/functions, native Details integration, editor data ownership, algorithms, constraints, source references and appropriate tests.

Don't paste duplicated README passages into both.

## Media replacements

See [Media Inventory](MEDIA_INVENTORY.md) and [media-slots.json](assets/media-slots.json). Each media placement has a type (**Image** or **GIF**), a reusable placeholder, reader-facing caption and a second visible capture-instruction line. Replace with genuine Prune Editor captures matching the instructions; do not fabricate Unreal interface results.

## Adding a topic

1. Add the matching `DESIGN.md` and `TECHNICAL.md` under a numbered section.
2. Update that section's `README.md` table row and the topic count in the root `README.md`.
3. Link to verified current source, and add a capture requirement if a visual helps.
4. Verify relative Markdown links, Design/Technical navigation, search and mobile layout.
5. Update the [Verification Matrix](VERIFICATION_MATRIX.md) with tests and actual results.

## Publishing

Configure GitHub repository Pages to deploy from `main` → `/docs`. Expected site: https://mippi-the-dork.github.io/Prune/ . Verify the actual deployment before claiming the website is live.

## Roadmap discipline

The [Roadmap](ROADMAP.md) treats v1.0.0 as released and other ideas as speculative. Don't invent historical releases, version promises or development dates.
