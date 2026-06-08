(function () {
  "use strict";

  // ---------- Theme toggle ----------
  const root = document.documentElement;
  const storedTheme = localStorage.getItem("ff-theme");
  if (storedTheme === "light" || storedTheme === "dark") {
    root.setAttribute("data-theme", storedTheme);
  }
  const toggle = document.getElementById("theme-toggle");
  if (toggle) {
    toggle.addEventListener("click", function () {
      const current =
        root.getAttribute("data-theme") ||
        (window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light");
      const next = current === "dark" ? "light" : "dark";
      root.setAttribute("data-theme", next);
      localStorage.setItem("ff-theme", next);
    });
  }

  // ---------- Copy-to-clipboard on code blocks ----------
  document.querySelectorAll("pre > code").forEach(function (code) {
    const pre = code.parentElement;
    if (!pre || pre.querySelector(".copy-btn")) return;
    const btn = document.createElement("button");
    btn.className = "copy-btn";
    btn.type = "button";
    btn.textContent = "Copy";
    btn.setAttribute("aria-label", "Copy code to clipboard");
    btn.addEventListener("click", function () {
      const text = code.innerText;
      const done = function () {
        btn.textContent = "Copied";
        btn.classList.add("copied");
        setTimeout(function () {
          btn.textContent = "Copy";
          btn.classList.remove("copied");
        }, 1400);
      };
      if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(text).then(done, function () {
          fallbackCopy(text);
          done();
        });
      } else {
        fallbackCopy(text);
        done();
      }
    });
    pre.appendChild(btn);
  });

  function fallbackCopy(text) {
    const ta = document.createElement("textarea");
    ta.value = text;
    ta.style.position = "fixed";
    ta.style.opacity = "0";
    document.body.appendChild(ta);
    ta.select();
    try { document.execCommand("copy"); } catch (e) { /* ignore */ }
    document.body.removeChild(ta);
  }

  // ---------- Live version refresh from GitHub Releases ----------
  // The build-time workflow substitutes 8.2.8 in the HTML; this
  // additionally refreshes the displayed version in the browser so the
  // site always reflects the very latest release without redeploying.
  const versionNodes = document.querySelectorAll("[data-version]");
  const downloadLink = document.getElementById("download-latest");
  if (versionNodes.length === 0) return;

  // Don't refetch more than once per hour per visitor.
  const CACHE_KEY = "ff-latest-release";
  const CACHE_TTL = 60 * 60 * 1000;
  let cached = null;
  try {
    const raw = localStorage.getItem(CACHE_KEY);
    if (raw) {
      const parsed = JSON.parse(raw);
      if (parsed && parsed.ts && Date.now() - parsed.ts < CACHE_TTL) {
        cached = parsed;
      }
    }
  } catch (e) { /* ignore */ }

  if (cached && cached.tag) {
    applyVersion(cached.tag, cached.url);
  } else {
    fetch("https://api.github.com/repos/fastfloat/fast_float/releases/latest", {
      headers: { Accept: "application/vnd.github+json" },
    })
      .then(function (r) { return r.ok ? r.json() : null; })
      .then(function (data) {
        if (!data || !data.tag_name) return;
        try {
          localStorage.setItem(
            CACHE_KEY,
            JSON.stringify({ ts: Date.now(), tag: data.tag_name, url: data.html_url })
          );
        } catch (e) { /* ignore */ }
        applyVersion(data.tag_name, data.html_url);
      })
      .catch(function () { /* offline / rate limited — keep build-time value */ });
  }

  function applyVersion(tag, url) {
    const clean = tag.replace(/^v/, "");
    versionNodes.forEach(function (el) {
      // Preserve a leading "v" if the original text used one.
      const wasV = (el.textContent || "").trim().startsWith("v");
      el.textContent = (wasV ? "v" : "") + clean;
    });
    if (downloadLink && url) downloadLink.href = url;
  }
})();
