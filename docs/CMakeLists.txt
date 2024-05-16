set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_HIDE_UNDOC_MEMBERS YES)
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
set(DOXYGEN_EXCLUDE_SYMBOLS "co_ecs::detail::*,*::detail::*")

set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_DISABLE_INDEX NO)
set(DOXYGEN_FULL_SIDEBAR NO)
set(DOXYGEN_HTML_EXTRA_STYLESHEET
  doxygen-awesome-css/doxygen-awesome.css
  doxygen-awesome-css/doxygen-awesome-sidebar-only.css
  doxygen-awesome-css/doxygen-awesome-sidebar-only-darkmode-toggle.css
)
set(DOXYGEN_HTML_EXTRA_FILES
  doxygen-awesome-css/doxygen-awesome-darkmode-toggle.js
  doxygen-awesome-css/doxygen-awesome-fragment-copy-button.js
  doxygen-awesome-css/doxygen-awesome-interactive-toc.js
)
set(DOXYGEN_HTML_HEADER header.html)
set(DOXYGEN_HTML_COLORSTYLE LIGHT)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
set(DOXYGEN_INTERACTIVE_SVG YES)

doxygen_add_docs(docs
  ${CMAKE_SOURCE_DIR}/README.md
  ${CMAKE_SOURCE_DIR}/include/
  COMMENT "Generate documentation"
)
