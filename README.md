# fast_float website

The source for <https://fastfloat.github.io/fast_float/>.

Static HTML, CSS, and a small JS file — no build step required. To preview
locally, serve this directory with any static file server, for example:

```bash
python3 -m http.server -d docs 8080
# then open http://localhost:8080/
```

## How the version number stays current

The displayed release version is kept fresh by two independent mechanisms:

1. **Build-time substitution.** The `Deploy GitHub Pages` workflow
   (`.github/workflows/pages.yml`) replaces every occurrence of the literal
   `8.2.7` in `index.html` with the latest GitHub release tag before
   publishing. The workflow runs on every push to `main` that touches
   `docs/**`, on every published release, and can be dispatched manually.

2. **Client-side refresh.** `assets/app.js` queries the GitHub Releases API
   on page load and overwrites any element marked with `data-version`. This
   means visitors see the very latest tag even between deploys.
