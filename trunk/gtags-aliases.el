;;; Aliases for partially preserving backwards compatability.
;;;
;;; We load gtags.el and then add aliases for common variables and functions

(require 'cl)

;;;; Backwards-compatability for missing functions in older clients.

;; Route calls to define-obsolete-function-alias to defalias if the former
;; function doesn't exist.
(if (not (fboundp 'define-obsolete-function-alias))
  (defalias 'define-obsolete-function-alias 'defalias))

;; Ignore calls to define-obsolete-variable-alias if the function doesn't exist
(if (not (fboundp 'define-obsolete-variable-alias))
  (defalias 'define-obsolete-variable-alias 'ignore))

;;;; Define aliases for wiki-documented functions
(define-obsolete-function-alias
  'google-feeling-lucky
  'gtags-feeling-lucky)
(define-obsolete-function-alias
  'google-find-tag
  'gtags-find-tag)
(define-obsolete-function-alias
  'google-first-tag
  'gtags-first-tag)
(define-obsolete-function-alias
  'google-next-tag
  'gtags-next-tag)
(define-obsolete-function-alias
  'google-show-tag-locations
  'gtags-show-tag-locations)
(define-obsolete-function-alias
  'google-show-tag-locations-regexp
  'gtags-show-tag-locations-regexp)
(define-obsolete-function-alias
  'google-tags-query-replace
  'gtags-query-replace)
(define-obsolete-function-alias
  'google-pop-tag
  'gtags-pop-tag)
(define-obsolete-function-alias
  'google-show-matching-tags
  'gtags-show-matching-tags)
(define-obsolete-function-alias
  'google-show-callers
  'gtags-show-callers)
(define-obsolete-function-alias
  'google-show-tag-locations-under-point
  'gtags-show-tag-locations-under-point)
(define-obsolete-function-alias
  'google-show-callers-under-point
  'gtags-show-callers-under-point)
(define-obsolete-function-alias
  'google-search-definition-snippets
  'gtags-search-definition-snippets)
(define-obsolete-function-alias
  'google-list-tags
  'gtags-list-tags)
(define-obsolete-function-alias
  'google-complete-tag
  'gtags-complete-tag)

;;;; Define aliases for variables from gtags.el
(define-obsolete-variable-alias
  'google-c-or-java-function-definition-regexp
  'gtags-c-or-java-function-definition-regexp)
(define-obsolete-variable-alias
  'google-c-or-java-function-regexp
  'gtags-c-or-java-function-regexp)
(define-obsolete-variable-alias
  'google-choose-server-and-get-tags-memoize-interval-seconds
  'gtags-choose-server-and-get-tags-memoize-interval-seconds)
(define-obsolete-variable-alias
  'google-choose-server-and-get-tags-memoize-result-count
  'gtags-choose-server-and-get-tags-memoize-result-count)
(define-obsolete-variable-alias
  'google-default-root
  'gtags-default-root)
(define-obsolete-variable-alias
  'google-filename-generators
  'gtags-filename-generators)
(define-obsolete-variable-alias
  'google-find-tag-history
  'gtags-find-tag-history)
(define-obsolete-variable-alias
  'google-gtags-call-graph-hosts
  'gtags-call-graph-hosts)
(define-obsolete-variable-alias
  'google-gtags-corpus
  'gtags-corpus)
(define-obsolete-variable-alias
  'google-gtags-extensions-to-languages
  'gtags-extensions-to-languages)
(define-obsolete-variable-alias
  'google-gtags-language-hosts
  'gtags-language-hosts)
(define-obsolete-variable-alias
  'google-gtags-ranking-methods
  'gtags-ranking-methods)
(define-obsolete-variable-alias
  'google-host-port-memoization-interval-seconds
  'gtags-host-port-memoization-interval-seconds)
(define-obsolete-variable-alias
  'google-host-port-memoization-result-count
  'gtags-host-port-memoization-result-count)
(define-obsolete-variable-alias
  'google-javadoc-url-template
  'gtags-javadoc-url-template)
(define-obsolete-variable-alias
  'google-multi-server-too-slow-threshold
  'gtags-multi-server-too-slow-threshold)
(define-obsolete-variable-alias
  'google-next-tags
  'gtags-next-tags)
(define-obsolete-variable-alias
  'google-previous-tags
  'gtags-previous-tags)
(define-obsolete-variable-alias
  'google-python-function-definition-regexp
  'gtags-python-function-definition-regexp)
(define-obsolete-variable-alias
  'google-source-root
  'gtags-source-root)
(define-obsolete-variable-alias
  'google-tags-default-mode
  'gtags-default-mode)
(define-obsolete-variable-alias
  'google-tags-history
  'gtags-history)
(define-obsolete-variable-alias
  'google-tags-language-options
  'gtags-language-options)
(define-obsolete-variable-alias
  'google-tags-modes-to-languages
  'gtags-modes-to-languages)
(define-obsolete-variable-alias
  'google-tags-multi-servers
  'gtags-multi-servers)
(define-obsolete-variable-alias
  'google-tags-output-mode
  'gtags-output-mode)
(define-obsolete-variable-alias
  'google-tags-output-options
  'gtags-output-options)
(define-obsolete-variable-alias
  'google-tags-print-trace
  'gtags-print-trace)
(define-obsolete-variable-alias
  'google-warning-multi-server-slow
  'gtags-warning-multi-server-slow)

;;;; Define aliases for other commonly used variables
(define-obsolete-variable-alias
  'google-default-root
  'gtags-default-root)
(define-obsolete-variable-alias
  'google-gtags-corpus
  'gtags-corpus)
(define-obsolete-variable-alias
  'google-tags-default-mode
  'gtags-default-mode)
(define-obsolete-variable-alias
  'google-tags-output-mode
  'gtags-output-mode)

(provide 'gtags-aliases)
