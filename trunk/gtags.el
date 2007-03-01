;;; Emacs Lisp Code for handling google tags
;;; Authors: Piaw Na, Arthur A. Gleckler, Leandro Groisman, Phil Sung
;;; Copyright Google Inc. 2004-2006

;;; This program is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU General Public License
;;; as published by the Free Software Foundation; either version 2
;;; of the License, or (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

;;; xemacs users MUST load etags.el
(defun google-xemacs()
  (string-match "XEmacs" emacs-version))

(if (google-xemacs)
    (progn
      (load "/home/build/public/eng/elisp/xemacs/etags")
      (require 'wid-browse)))

(defconst google-gtags-client-version 1
  "Version number for this (emacs) gtags client.")

(defconst google-gtags-protocol-version 2
  "Version number for the protocol that this client speaks.")

(require 'etags)
(require 'cl)
(require 'google-emacs-utilities)
(require 'tree-widget)

;;******************************************************************************
;; Basic Configuration

(defvar google-gtags-language-hosts
  '(("c++" . (("gtags.google.com" . 2223)))
    ("java" . (("gtags.google.com" . 2224)))
    ("python" . (("gtags.google.com" . 2225))))
  "Each language is paired with a list of hostname, port pairs. To set up 
multiple GTags servers, add additional hostname, port pairs and set
google-tags-multi-servers to true")

(defvar google-gtags-call-graph-hosts
  '(("c++" . (("gtags.google.com" . 2223)))
    ("java" . (("gtags.google.com" . 2224)))
    ("python" . (("gtags.google.com" . 2225))))
  "Similar to google-gtags-language-hosts, but these are for call graph
servers")

(defvar google-tags-language-options
  '(("c++" c++-mode)
    ("java" java-mode)
    ("python" python-mode))
  "Each language is identified by a string which can either be entered
interactively or passed as an argument to any of the google-*
functions that expect them.  Maps each string to a mode.")

(defvar google-tags-modes-to-languages
  (cons '(jde-mode . "java")
        (mapcar (lambda (association)
                  (cons (cadr association)
                        (car association)))
                google-tags-language-options)))

(defvar google-gtags-extensions-to-languages
  '()
  "an alist mapping each file extension to the corresponding
language")

(defvar google-tags-default-mode 'c++-mode
  "Mode for the language to use to look up the Gtags server when
the buffer and file extension don't map to a Gtags server.  Must
be one of the symbols c++-mode, java-mode, or python-mode.")

;;;;;;;;;;;;;;
;; Debug functions

(defvar gtags-print-trace nil)

(defun pri (text)
  (if gtags-print-trace
      (princ text (get-buffer-create "**dbg**"))))

(defun pril (text)
  (when gtags-print-trace
    (princ text (get-buffer-create "**dbg**"))
    (terpri (get-buffer-create "**dbg**"))))

(defun google-get-buffer-context (buffer)
  "given buffer, get context (mode and file extension)"
  (let* ((mode (save-excursion (set-buffer buffer) major-mode))
         (file-extension (google-buffer-file-extension buffer)))
    (cons mode file-extension)))

;;;;;;;;;;;;;;;;;;
;; Data definition

(defun alist-get (key lst)
  "Returns the value associated with KEY in a list that looks
like ((key value) ...), or nil if nothing matches KEY."
  (let ((matching-entry (assoc key lst)))
    (and matching-entry (cadr matching-entry))))

(defun response-signature (response)
  "Extracts (server-start-time sequence-number) from a server
response (this is a unique identifier for the request)."
  (let ((server-start-time (alist-get 'server-start-time response))
        (sequence-number (alist-get 'sequence-number response)))
    `(,@server-start-time ,sequence-number)))

(defun response-value (response)
  "Extracts the 'value' from a server response.  This is a list of
tagrecords, for commands that return tags."
  (alist-get 'value response))

;; Tagrecord stores the information associated with a single matching
;; tag.
(defun tagrecord-tag (tagrecord)       (alist-get 'tag tagrecord))
(defun tagrecord-snippet (tagrecord)   (alist-get 'snippet tagrecord))
(defun tagrecord-filename (tagrecord)  (alist-get 'filename tagrecord))
(defun tagrecord-lineno (tagrecord)    (alist-get 'lineno tagrecord))
(defun tagrecord-offset (tagrecord)    (alist-get 'offset tagrecord))
(defun tagrecord-directory-distance (tagrecord)
  (alist-get 'directory-distance tagrecord))

;;;;;;;;;;;;;;;;;;;;
;; Tag history stack

(defvar google-tags-history nil
  "a stack of visited locations")

(defun google-pop-tag ()
  "Pop back up to the previous tag."
  (interactive)
  (if google-tags-history
      (let* ((google-tag-history-entry (car google-tags-history))
             (buffname (car google-tag-history-entry))
             (pt (cdr google-tag-history-entry)))
        (setq google-tags-history (cdr google-tags-history))
        (switch-to-buffer buffname)
        (goto-char pt))
    (error "No tag to pop!")))

(defun google-push-tag ()
  "Push current location onto the tag stack"
  (interactive)
  (push-mark)
  (let ((currloc (cons (buffer-name) (point))))
    (setq google-tags-history (cons currloc google-tags-history))))


;;;;;;;;;;;;;;;;;
;; Tag navigation

(defalias 'google-first-tag 'google-feeling-lucky)

(defvar google-next-tags nil
  "List of gtags matches remaining for use with `google-next-tag'.
It includes current tag as first element")

(defvar google-previous-tags nil
  "List of gtags matches previously visited with `google-next-tag'.")

(defun google-feeling-lucky (tagname &optional language)
  "Jump to the first exact match for TAGNAME.  Optionally specify
LANGUAGE to check (if omitted, the major mode and file extension
are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (and current-prefix-arg
                                    (google-prompt-language))))
                 (list (google-prompt-tag "Tag: " language)
                       language)))
  (let ((matches (google-get-tags 'lookup-tag-exact
                                  `((tag ,tagname)
                                    (language ,language)
                                    (callers nil)
                                    (current-file
                                     ,(google-get-current-relative-path)))
                                  :ranked-p t
                                  :error-if-no-results-p t)))
    (setq google-next-tags matches)
    (setq google-previous-tags nil)
    (google-visit-tagrecord (car matches) t)))

(defun google-next-tag ()
  "Jump to the next tag in the results produced by `google-feeling-lucky'."
  (interactive)
  (if (cdr google-next-tags)
      (let ((this-match (car google-next-tags)))
        (setq google-next-tags (cdr google-next-tags))
        (setq google-previous-tags (cons this-match google-previous-tags))
        (google-visit-tagrecord (car google-next-tags)))
    (error "No more matches found!")))

(defun google-previous-tag ()
  "Jump to the previous tag in the results produced by `google-feeling-lucky'."
  (interactive)
  (if google-previous-tags
      (let ((previous-match (car google-previous-tags)))
        (setq google-next-tags (cons previous-match google-next-tags))
        (setq google-previous-tags (cdr google-previous-tags))
        (google-visit-tagrecord (car google-next-tags)))
    (error "No more matches found!")))

;;;;;;;;;;;;;;;;;
;; Tag completion

(defun google-completion-collection-function (buffer
                                              string
                                              predicate
                                              operation-type
                                              &optional
                                              language)
  "completing-read calls this function with three parameters to
get a completion.  Optionally, LANGUAGE overrides the default
language used for the tags lookup."
  (case operation-type
    ;; nil specifies try-completion.  Return the completion of STRING,
    ;; or t if the string is an exact match already, or nil if the
    ;; string matches no possibility.
    ((nil)
     (let ((completions (google-all-matching-completions string
                                                         predicate
                                                         buffer
                                                         language)))
       (and completions
            (let ((first-completion (car completions)))
              (or (and (null (cdr completions))
                       (string-equal string first-completion))
                  (google-longest-common-prefix first-completion
                                                (car (last completions))))))))
    ;; t specifies all-completions.  Return a list of all possible
    ;; completions of the specified string.
    ((t) (google-all-matching-completions string predicate buffer language))
    ;; lambda specifies a test for an exact match.  Return t if the
    ;; specified string is an exact match for some possibility; nil
    ;; otherwise.
    ((lambda) (google-get-tags 'lookup-tag-exact
                               `((tag ,string)
                                 (language ,language)
                                 (callers nil)
                                 (current-file
                                  ,(google-get-current-relative-path)))
                               :buffer buffer
                               :filter predicate))))

(defun google-all-matching-completions (prefix
                                        predicate
                                        &optional
                                        buffer
                                        language)
  "Returns a list of all tags that begin with PREFIX.  Optionally
override the language to search with LANGUAGE."
  (remove-duplicates (google-get-tags 'lookup-tag-prefix-regexp
                                      `((tag ,prefix)
                                        (language ,language)
                                        (callers nil)
                                        (current-file
                                         ,(google-get-current-relative-path)))
                                      :buffer (or buffer (current-buffer))
                                      :filter predicate
                                      :map #'tagrecord-tag)
                     :test #'equal))

(defun google-complete-tag ()
  "Perform gtags completion on the text around point."
  (interactive)
  (let ((tag (google-tag-under-cursor)))
    (unless tag
      (error "Nothing to complete"))
    (let* ((start (progn (search-backward tag) (point)))
           (end (progn (forward-char (length tag)) (point)))
           (completions (google-all-matching-completions tag nil)))
      (cond ((null completions)
             (message "Can't find any completions for \"%s\"" tag))
            ((null (cdr completions))
             (if (string-equal tag (car completions))
                 (message "No more completions.")
               (delete-region start end)
               (insert (car completions))))
            (t
             (let ((longest-common-prefix
                    (google-longest-common-prefix (car completions)
                                                  (car (last completions)))))
               (if (string-equal tag longest-common-prefix)
                   (progn
                     (message "Making completion list...")
                     (with-output-to-temp-buffer "*Completions*"
                       (display-completion-list completions))
                     (message "Making completion list...%s" "done"))
                 (delete-region start end)
                 (insert longest-common-prefix))))))))

;;;;;;;;;;;;;;;;;
;; Main functions

(defun google-get-tags-and-show-results (command-type
                                         parameters
                                         &optional
                                         buffer-name
                                         caller-p)
  "Find tag server, fetch and process the tags, and display the results.
COMMAND-TYPE is a symbol with the command head (see the protocol
spec for allowable values).  PARAMETERS is a list of two-item
lists containing name-value pairs to pass along with the
command.If BUFFER-NAME is non-nil, the results are displayed in a
buffer of that name, else a new buffer is created.  CALLER-P, if
present, specifies that we're to search for callers."
  (google-tags-show-results
   (google-get-tags command-type
                    parameters
                    :caller-p caller-p
                    :ranked-p t
                    :error-if-no-results-p t)
   (or buffer-name
       ;; First try to name buffer using tag, then fall back to file
       (google-make-gtags-buffer-name
        (or (alist-get 'tag parameters)
            (alist-get 'file parameters))))))

(defun* google-get-tags (command-type
                         parameters
                         &key
                         caller-p
                         ranked-p
                         buffer
                         error-if-no-results-p
                         filter
                         map)
  (pril "google-get-tags")
  (let* ((buffer (or buffer (current-buffer)))
         (matches (google-get-tags-from-server command-type
                                               parameters
                                               caller-p
                                               buffer))
         (matches-filtered (if filter
                               (google-filter filter matches)
                             matches))
         (matches-ranked (if ranked-p
                             (google-rank-tags matches-filtered
                                               buffer)
                           matches-filtered))
         (matches-mapped (if map
                             (mapcar map matches-ranked)
                           matches-ranked)))
    (if (and error-if-no-results-p
             (null matches-mapped))
        (error "No tag found!")
      matches-mapped)))

(defvar google-tags-output-options
  '(("standard" google-show-tags-option-window-standard)
    ("single-line" google-show-tags-option-window-grep-style)
    ("single-line-grouped" google-show-tags-option-window-grouped)))

(defvar google-tags-output-mode 'standard)

(defun google-gtags-output-mode ()
  "Change tags selection window format."
  (interactive)
  (let ((mode (completing-read "Choose Mode (standard): "
                               google-tags-output-options
                               nil
                               t
                               nil
                               nil
                               "standard")))
    (setq google-tags-output-mode (intern mode))))


(defun google-show-tags-option-window (matches
                                       buffer-name
                                       current-directory)
  (let ((output-formatter-entry
         (assoc (symbol-name google-tags-output-mode)
                google-tags-output-options)))
    (if output-formatter-entry
        (let ((buffer (get-buffer-create buffer-name)))
          (set-buffer buffer)
          (toggle-read-only 0)
          (erase-buffer)
          (google-browse-tags-mode)
          (set (make-local-variable 'truncate-lines) t)
          (let ((window (display-buffer buffer)))
            (save-excursion
              (funcall (cadr output-formatter-entry)
                       matches
                       buffer
                       current-directory
                       window))
            (set (make-local-variable 'default-directory) current-directory)
            (toggle-read-only 1)
            (google-show-warning-if-multi-server-is-too-slow)
	    (select-window window)))
      (error "No such output mode %s." google-tags-output-mode))))

(defun google-show-warning-if-multi-server-is-too-slow ()
  (when google-warning-multi-server-slow
    (message (concat "Multi server mode too slow?  Consider "
                     "using M-x google-tags-set-single-data-server RET"))
    (setq google-multi-server-slow nil)))

(defun google-show-tags-option-window-grep-style (matches
                                                  buffer
                                                  current-directory
                                                  window)
  (google-show-tags-option-window-single-line-inner
   matches
   buffer
   current-directory
   window
   nil))

(defun google-show-tags-option-window-grouped (matches
                                               buffer
                                               current-directory
                                               window)
  (google-show-tags-option-window-single-line-inner
   matches
   buffer
   current-directory
   window
   t))

(defun google-show-tags-option-window-single-line-inner (matches
                                                         buffer
                                                         current-directory
                                                         window
                                                         grouped-p)
  (let ((last-filename nil))
    (dolist (tagrecord matches)
      (let ((prop-start (point))
            (tag (tagrecord-tag tagrecord))
            (lineno (tagrecord-lineno tagrecord))
            (snippet (google-trim-whitespace (tagrecord-snippet tagrecord)))
            (filename (tagrecord-filename tagrecord)))
        (if grouped-p
            (if (not (equal filename last-filename))
                (progn
                  (princ filename buffer)
                  (terpri buffer)
                  (setq last-filename filename)))
          (princ (format "%s:%d: " filename lineno) buffer))
        (if grouped-p
            (princ "  " buffer))
        (let ((snippet-start (point)))
          (princ snippet buffer)
          ;; Bold every occurrence of tag.
          (do ((match-start (string-match tag snippet)
                            (string-match tag snippet (1+ match-start))))
              ((not match-start) nil)
            (add-text-properties (+ snippet-start match-start)
                                 (+ snippet-start match-start (length tag))
                                 '(face bold))))
        (add-text-properties
         prop-start
         (point)
         `(gtag ,tagrecord
           help-echo "mouse-1: go to this occurrence"
           mouse-face secondary-selection))
        (terpri buffer)
        (if (google-xemacs)
            (let ((xemacs-region (make-extent prop-start (point))))
              (set-extent-property xemacs-region
                                   'mouse-face
                                   'widget-button-face)))))))

(defun google-show-tags-option-window-standard (matches
                                                buffer
                                                current-directory
                                                window)
  (let ((text-width (1- (window-width window))))
    (do ((matches matches (cdr matches))
         (face 'default
               (if (eq face 'default) 'highlight 'default)))
        ((null matches))
      (let* ((tagrecord (car matches))
             (prop-start (point))
             (tag (tagrecord-tag tagrecord))
             (snippet (tagrecord-snippet tagrecord))
             (filename (tagrecord-filename tagrecord)))
        (princ
         (format (format "%s%%%ds"
                         tag
                         (- text-width (length tag)))
                 filename)
         buffer)
        (terpri buffer)
        (princ snippet buffer)
        (princ (make-string (max 0 (- text-width (length snippet))) ?\ ) buffer)
        (add-text-properties prop-start
                             (point)
                             `(face ,face
                               gtag ,tagrecord
                               help-echo "mouse-1: go to this occurrence"
                               mouse-face secondary-selection))
        (terpri buffer)
        (if (google-xemacs)
            (let ((xemacs-region (make-extent prop-start (point))))
              (set-extent-property xemacs-region 'face face)
              (set-extent-property xemacs-region
                                   'mouse-face
                                   'widget-button-face)))))))

(defun google-tags-show-results (matches
                                 buffer-name)
  "Given matches, show results."
  (google-push-tag)                     ; Save actual position.
  (let ((current-directory default-directory))
    (cond ((or (= (length matches) 1)
               (and google-auto-visit-colocated-tags
                    (google-matches-colocated-p matches)))
           (google-visit-tagrecord (car matches) nil nil))
          (t (google-show-tags-option-window matches
                                             buffer-name
                                             current-directory)))))

(defcustom google-auto-visit-colocated-tags nil
  "If turned on, whenever all the entries are in the same file, jump
to them.")

(defun google-matches-colocated-p (matches)
  "Return t if all the tag matches in MATCHES are in the same file.
Does this by checking if any files don't match the first entry."
  (let ((firstfile (tagrecord-filename (car matches))))
    (not (find-if (lambda (tagrecord)
                    (not (string= firstfile
                                  (tagrecord-filename tagrecord))))
                  (cdr matches)))))

(defvar google-find-tag-history nil
"History of tags prompted for in Gtags.")

(defconst google-javadoc-url-template
  "https://www.corp.google.com/~engdocs/%s.html")

(defun google-visit-javadoc (tagrecord)
  "Visit the Javadoc corresponding to TAGRECORD."
  (let* ((gtags-pathname (tagrecord-filename tagrecord))
         (javadoc-pathname
          (concat (file-name-directory gtags-pathname)
                  (file-name-nondirectory
                   (file-name-sans-extension gtags-pathname)))))
    (browse-url
     (format google-javadoc-url-template javadoc-pathname)
     t)))

(defun google-get-tag-file (tag)
  "Returns the file containing the tag"
  (google-filename-after-first-slash (tagrecord-filename tag)))

;;******************************************************************************
;; Servers

(defvar google-tags-multi-servers nil)

(defun google-tags-set-multi-server ()
  (interactive)
  (setq google-tags-multi-servers t)
  (message "Gtags is now in multi-server mode."))

(defun google-tags-set-single-server ()
  (interactive)
  (setq google-tags-multi-servers nil)
  (message (concat
            "Gtags is now in single server mode.  To switch back, "
            "run M-x google-tags-set-multi-server.")))

(defun google-make-gtags-servers-alist (caller-p language)
  "Given the language and whether call graph is requested, return a list of
servers that can handle the request"
  (cdr (assoc language (if caller-p 
			   google-gtags-call-graph-hosts 
			 google-gtags-language-hosts))))

(defun google-mode-to-language (mode)
  (rassoc* mode google-tags-language-options :key #'car))
;;******************************************************************************
;; Connection Manager

;;;;;;;;;;;;;;;;;;;;;;;
;; Gtags server constants

(defconst google-gtags-connection-attempts 3)

(defconst gtags-server-buffer-name "*TAGS server*"
  "buffer used for communication with gtags server")

(defconst google-genfile-pattern
  "\\(genfiles/.*\\)"
  "Pattern for recognizing genfiles and stripping out the obj/gcc2-3 stuff.")

;;;;;;;;;;;;;;;;;;;;;;;;
;; Connection management

(defun google-get-tags-from-server (command-type
                                    parameters
                                    caller-p
                                    buffer)
  "Choose a server and return the tags.
See `google-get-tags-and-show-results' for a description of the parameters."
  (google-choose-server-and-get-tags command-type
                                     parameters
                                     caller-p
                                     (google-get-buffer-context buffer)
                                     google-gtags-connection-attempts))

(defun google-choose-server-and-get-tags (command-type
                                          parameters
                                          caller-p
                                          context
                                          attempts)
  "Choose best server, issue a request, and return only the
matching-tags list."
  (flet ((issue-command ()
           (google-issue-command
            command-type
            parameters
            (google-tags-find-host-port-pair caller-p context parameters))))
    (do ((response (issue-command) (issue-command))
       (attempts-left attempts
                      (1- attempts-left)))
      ((or (listp response) (= attempts-left 0))
       (if (listp response)
           (response-value response)
         (error "Cannot connect to server.  Try again in a few minutes.")))
    (pri "ATTEMPTs left: ")
    (pri attempts)
    (pri " -- ")
    (pril response)
    (memoize-forget 'google-tags-find-host-port-pair))))

(defvar google-choose-server-and-get-tags-memoize-result-count 100)
(defvar google-choose-server-and-get-tags-memoize-interval-seconds (* 60 60))

(memoize 'google-choose-server-and-get-tags
         google-choose-server-and-get-tags-memoize-result-count
         google-choose-server-and-get-tags-memoize-interval-seconds)

;;;;;;;;;;;;;;;;;
;; Finding server

(defun google-tags-find-host-port-pair (caller-p context &optional parameters)
  "Given CONTEXT (mode and file extension) and CALLER-P (whether
we're searching for callers), return the host/port pair for the
fastest server.

If provided, language information in PARAMETERS (list of
name/value pairs as specified by the protocol) may override the
the server that would otherwise be selected based on the
context."
  (pril "Find host port pair")
  (let ((pairs (google-tags-find-host-port-pairs caller-p context parameters))
        (selected-pair nil)
        (best-time google-starting-best-time)
        (begin-find-host-port-pair (current-time)))
    (if google-tags-multi-servers
        (dolist (pair pairs)
          (let* ((before (current-time))
                 (temp-buffer-name "*gtags-server-response-time*")
                 (succeed (google-send-string-to-server
                           (with-output-to-string
                             (prin1 (google-make-command 'ping '())))
                           temp-buffer-name
                           pair
                           gtags-ping-connection-timeout))
                 (after (current-time))
                 (elapsed-time (google-elapsed-time before after)))
            (pri "Server response: ")
            (if succeed (pril (google-buffer-string temp-buffer-name)))
            (if (and succeed
                     (equal (response-value
                             (google-server-response temp-buffer-name))
                            t)
                     (< elapsed-time best-time))
                (progn
                  (setq selected-pair pair)
                  (setq best-time elapsed-time)
                  (pri "New pair chosen: ")
                  (pri selected-pair)
                  (pri " -- temp ")
                  (pril elapsed-time))
              (if succeed
                  (progn
                    (pri elapsed-time)
                    (pril " too slow"))
                (pril "failed!")))))
      (setq selected-pair (car pairs)))
    (pri "Total elapsed time choosing the server")
    (pril (google-elapsed-time begin-find-host-port-pair (current-time)))
    (if (> (google-elapsed-time begin-find-host-port-pair (current-time))
           google-multi-server-too-slow-threshold)
        (setq google-warning-multi-server-slow t))
    selected-pair))

(defconst google-starting-best-time 20.0)
(defvar google-warning-multi-server-slow nil)
(defvar google-multi-server-too-slow-threshold 8)
(defvar google-host-port-memoization-result-count 100)
(defvar google-host-port-memoization-interval-seconds 6000)

(memoize 'google-tags-find-host-port-pair
         google-host-port-memoization-result-count
         google-host-port-memoization-interval-seconds)

(defun google-guess-language (context &optional parameters)
  "Given the context (a pair of a mode and a filename extension) and
the parameters to be sent to the Gtags server, return the best guess
at the programming language being used."
  (or (alist-get 'language parameters)
      (cdr (assq (car context)
                 google-tags-modes-to-languages))
      (let ((file-extension (cdr context)))
        (and file-extension
             (cdr (assoc file-extension
                         google-gtags-extensions-to-languages))))
      (car (google-mode-to-language google-tags-default-mode))))

(defun google-tags-find-host-port-pairs (caller-p context &optional parameters)
  "Given CONTEXT (mode and file extension) and CALLER-P (whether
we're searching for callers), return a list of host/port pairs
for all servers that could answer the request.

If provided, language information in PARAMETERS (list of
name/value pairs as specified by the protocol) may override the
the servers that would otherwise be selected."
  (let ((language (google-guess-language context parameters)))
    (google-make-gtags-servers-alist caller-p language)))

(defun google-elapsed-time (start end)
  "Elapsed time between 'current-time's"
  (+ (* (- (car end) (car start)) 65536.0)
     (- (cadr end) (cadr start))
     (/ (- (caddr end) (caddr start)) 1000000.0)))

(defun google-server-response (buff)
  "Returns the server response from a buffer, i.e. the first
s-exp in the buffer."
  (save-excursion
    (set-buffer buff)
    (goto-char (point-min))
    (read (current-buffer))))

(defun google-buffer-string (buff)
  (save-excursion
    (set-buffer buff)
    (buffer-string)))

;;;;;;;;;;;;;;;;;;;
;; Managing streams

(defun google-command-boilerplate ()
  "List of the standard parameters that have to go with every
request."
  (let ((client-type-string (if (google-xemacs)
                                "xemacs"
                              "gnu-emacs")))
    `((client-type ,client-type-string)
      (client-version ,google-gtags-client-version)
      (protocol-version ,google-gtags-protocol-version))))

(defun google-make-command (command-type extra-params)
  "Makes a command with the specified type and parameters (and
the boilerplate client/protocol versions) and returns it."
  `(,command-type ,@(google-command-boilerplate)
                  ,@extra-params))

(defun google-issue-command (command-type extra-params host-port-pair)
  "Issues a command with the specified type and parameters (and
the boilerplate client/protocol versions) and returns the result."
  (google-send-command-to-server (google-make-command command-type
                                                      extra-params)
                                 host-port-pair))

(defun google-send-command-to-server (command host-port-pair)
  "Sends an s-exp to the server and returns the response as a s-exp."
  (if (google-send-string-to-server (with-output-to-string
                                      (prin1 command))
                                    gtags-server-buffer-name
                                    host-port-pair
                                    gtags-query-connection-timeout)
      (google-server-response gtags-server-buffer-name)
    'error))

(defun google-send-string-to-server (command-string
                                     response-buffer-name
                                     host-port-pair
                                     connection-timeout-limit)
  "Send a command (string) to the tag server, then wait for the
response and write it to a buffer.

COMMAND-STRING: string we will send to the server

RESPONSE-BUFFER-NAME: buffer where the server response will be stored

HOST-PORT-PAIR: (host . port) server network address

CONNECTION-TIMEOUT-LIMIT: How many times the server will be polled.  The actual
limit is (connection-timeout-limits * google-sleep-time-between-server-polls)
secs.

It returns nil if something goes wrong."
  (pril "google send tag to server")
  (pri "buffer: ")
  (pril response-buffer-name)
  (pri "host-port: ")
  (pril host-port-pair)
  (pri (car host-port-pair))
  (pri " ")
  (pril (cdr host-port-pair))
  (pri "command-string: ")
  (pril command-string)
  (if (get-buffer response-buffer-name) (kill-buffer response-buffer-name))
  (let* ((temp-response-buffer-name (generate-new-buffer-name "*gtags-temp*"))
         (stream-is-ready nil)
         (host (car host-port-pair))
         (port (cdr host-port-pair))
         (network-stream-name (symbol-name (gensym)))
         (network-stream (condition-case stream-error-code
                             (open-network-stream network-stream-name
                                                  temp-response-buffer-name
                                                  host
                                                  ;; On some XEmacs,
                                                  ;; open-network-stream
                                                  ;; requires the port
                                                  ;; to be given as a
                                                  ;; string, and GNU
                                                  ;; Emacs doesn't
                                                  ;; mind
                                                  (number-to-string port))
              (error (pril "Error gtags-0044")
                     (pril stream-error-code)
                     nil))))
    (and network-stream ;; Keep going if the stream could be opened
         ((lambda (x) t)
          (condition-case stream-error-code
              (process-send-string network-stream-name
                                   (concat command-string "\r\n"))
            (error (pril "Error gtags-0045")
                   (pril stream-error-code)
                   (setq network-stream nil))))
         network-stream ;; Keep going if the string could be sent.
         (wait-until-stream-is-ready network-stream
                                     connection-timeout-limit)
         (google-rename-buffer temp-response-buffer-name
                               response-buffer-name))))

(defun network-stream-ready (network-stream)
  "The network connection is ready to be read if it is closed"
  (case (process-status network-stream)
    ('open nil)
    ('closed t)))

(defun wait-until-stream-is-ready (network-stream connection-timeout-limit)
  "Wait for the connection to be ready
timeout = google-sleep-time-between-server-polls * connection-timeout

Return nil in case of timeout"
  (pril "I will wait for the server")
  (let ((stream-is-ready nil)
        (times-polled 0))
    (while (and (not stream-is-ready) (< times-polled connection-timeout-limit))
      (setq stream-is-ready (network-stream-ready network-stream))
      (incf times-polled)
      (if (not stream-is-ready)
          (sleep-for gtags-sleep-time-between-server-polls)))
    (pri "Waiting finished, stream ready?")
    (pril stream-is-ready)
    stream-is-ready))

(defconst gtags-sleep-time-between-server-polls 0.01)
(defconst gtags-query-connection-timeout 1000) ;; 10 seconds
(defconst gtags-ping-connection-timeout 200)   ;; 2 seconds

;;******************************************************************************
;; Rank

(defun google-rank-tags (matches buf)
  "Given the matches of a query, possibly order them on the basis
of google-bias-to-header-files"
  (cond ((and google-bias-to-header-files
              (= (length matches) 2)
              (let* ((match1 (car matches))
                     (match2 (cadr matches))
                     (file1 (tagrecord-filename match1))
                     (file2 (tagrecord-filename match2)))
                (and (equal (file-name-sans-extension file1)
                            (file-name-sans-extension file2))
                     (or (equal (file-name-extension file1) "h")
                         (equal (file-name-extension file2) "h")))))
         (if (equal (file-name-extension (tagrecord-filename (car matches)))
                    "h")
             (list (car matches))
           (list (cadr matches))))
        (t matches)))

(defcustom google-bias-to-header-files nil
  "If turned on, whenever there are only two entries and there's just a header
file and a .cc file, we ignore the .cc file and jump straight to the header.")

;;******************************************************************************
;; Tree View

(defun google-show-call-tree (tagname)
  "Show the callers of of a tag, and the callers of the callers, etc, in
a tree widget"
  (interactive (list (google-prompt-tag "Tag: ")))
  (let ((orig-buf (current-buffer))
        (tag-identifier (concat (google-get-enclosing-class
                                 (buffer-file-name))
                                 "." tagname))
        (buf (get-buffer-create (concat "GTags call graph for "
                                        tagname))))
    (switch-to-buffer buf)
    (erase-buffer)
    (insert "Warning: This gtag-based calltree tends to over-include items.\n")
    (widget-create 'tree-widget
                   :tag tag-identifier
                   :dynargs 'google-gtags-get-children-from-tree
                   :has-children t
                   :sig (cons orig-buf tagname))
    (use-local-map widget-keymap)
    (widget-setup)))

(defun google-calling-function-to-tagname (function-name)
  "Takes a FUNCTION-NAME and processes it so that it transforms
into a tag name"
  (let ((simple-identifier "\\([[:alnum:]\.-_]+\\)" ))
    (if (string-match (concat simple-identifier "::" simple-identifier)
                      function-name)
        (let ((match (match-string 2 function-name)))
          (set-text-properties 0 (length match)
                               nil match)
          match)
      function-name)))

(defun google-gtags-get-children-from-tree (tree)
  (let* ((sig (widget-get tree :sig))
         (tagname (cdr sig)))
    (save-window-excursion
      (save-excursion
        (if (bufferp (car sig))
            (set-buffer (car sig))
          (google-visit-tagrecord (car sig) nil nil)
          (find-file (google-get-path-from-match (car sig))))
        (message "Getting list of matches for tag %s...." tagname)
        (let* ((matches
                (google-get-tags 'lookup-tag-exact
                                 `((tag ,tagname)
                                   (callers t))
                                 :caller-p t
                                 :ranked-p t))
               (count 0))
          (message "Matches retrieved")
          (mapcan
           (lambda (match)
             (let* ((file (google-get-path-from-match match))
                    (calling-function
                     (google-get-calling-function-from-match match))
                    (display-name
                     (google-get-calling-identifier-from-match match))
                    (calling-tagname
                     (google-calling-function-to-tagname calling-function)))
               ;; Filter out the case where child tag = parent tag,
               ;; which can happens with gtags.
               (unless (equal (widget-get
                               (widget-get tree :node) :tag)
                              display-name)
                 (incf count)
                 (message "Investigated match %s" calling-function)
                 (list
                  (list 'tree-widget
                        :node `(push-button
                                :tag ,display-name
                                :format "%[%t%]\n"
                                :sig ,(cons match calling-tagname)
                                :caller ,match
                                :notify google-gtags-caller-notify)
                        :dynargs 'google-gtags-get-children-from-tree
                        :sig (cons match calling-tagname)
                        :has-children t)))))
           matches))))))

(defun google-gtags-caller-notify (widget child &optional event)
  (google-visit-tagrecord (widget-get widget :caller) nil nil))

(defun google-get-path-from-match (match)
  (let* ((filename (tagrecord-filename match))
         (actual-filename
          (expand-file-name
           (concat (file-truename
                    (find-google-source-root default-directory))
                   filename))) ;; ()
         (default-filename (expand-file-name
                            (concat (file-truename google-default-root)
                                    filename))))
    (google-select-from-files (google-generate-potential-filenames
                               actual-filename
                               default-filename))))

;; Regexps are the union of java and c++.  This may cause us to
;; do some bad things, and in that case we need to split things up.
;;
;; The generated regexp can get confused with catch statements.  This
;; is handled in a special way later.
(let* ((space "[[:space:]\n]") ;; Contrary to the docs, [:space:]
                               ;; doesn't match newline.
       (identifier-regexp "[[:alnum:]_\\.-:*&]+")
       (identifier-pair-regexp (concat "\\(?:\\(?:const\\|final"
                                       space "*" "\\)?"
                                       identifier-regexp space "+"
                                       identifier-regexp "\\)"))
       (comma-separator-regexp (concat space "*," space "*"))
       (argument-list-regexp (concat
                              ;; 0 - many arguments with comma
                              "(" space "*" "\\(?:" identifier-pair-regexp
                              comma-separator-regexp "\\)*"
                              ;; 0 - one argument without comma
                              identifier-pair-regexp "?" space "*)"))
       (throws-regexp (concat "throws" space "+" "\\(?:" identifier-regexp
                              comma-separator-regexp "\\)*" "\\(?:"
                              identifier-regexp "\\)?"))
       (function-regexp (concat space "+\\(" identifier-regexp
                                "\\)" space "*"
                                argument-list-regexp space "*"
                                "\\(?:" throws-regexp "\\)?"))
       (function-declaration-regexp (concat function-regexp
                                            space "*"
                                            "\\(?:=\\|;\\|{\\)"))
       (function-definition-regexp (concat function-regexp
                                           space "*{"))
       (py-function-regexp (concat "def" space "+\\(" identifier-regexp
                                   "+\\)")))
  (defvar google-c-or-java-function-regexp function-declaration-regexp)
  (defvar google-c-or-java-function-definition-regexp
    function-definition-regexp)
  (defvar google-python-function-definition-regexp py-function-regexp))

(defun google-get-appropriate-definition-regexp ()
  "Get the regexp that is appropriate for the buffer mode (a
python regexp for python mode, java regexp for java mode, etc."

  (if (eq major-mode 'python-mode)
      google-python-function-definition-regexp
    google-c-or-java-function-definition-regexp))

(defun google-get-appropriate-declaration-regexp ()
  "Get the regexp that is appropriate for the buffer mode (a
python regexp for python mode, java regexp for java mode, etc."
  (if (eq major-mode 'python-mode)
      google-python-function-definition-regexp
    google-c-or-java-function-regexp))

(defun google-line-number-at-pos (&optional pos)
  "This is a replicating the functionality of v22 function
`line-number-at-pos'."

  (save-excursion
    (when pos (goto-char pos))
    (+ (count-lines (point-min) (point))
       (if (= (current-column) 0) 1 0))))

(defun google-get-enclosing-identifier (filename)
  "In the current buffer, at the current point, get an identifier
such as FooClass.barMethod that identifies the current point"
  (let ((function-name (google-get-enclosing-function))
        (simple-identifier "\\([[:alnum:]\.-_]+\\)" ))
    (concat
     (if (string-match (concat simple-identifier "::" simple-identifier)
                       function-name)
         (concat (match-string 1 function-name) "."
                 (match-string 2 function-name))
       (concat (google-get-enclosing-class filename) "."
               function-name)) ":"
     (int-to-string (google-line-number-at-pos)))))

(defun google-get-enclosing-class (filename)
  "Transform the given filename into a classname"
  (condition-case err
      (file-name-sans-extension (file-name-nondirectory filename))
    (error
     (concat "You must be visiting a Java/C++ or Python file"
             "to use this functionality."))))

(defun google-get-function-at-same-line ()
  "If there is a function on the same line, then return it, otherwise
return nil"
  (save-excursion
    (let ((initial-line-number (google-line-number-at-pos)))
      (search-forward-regexp
       (google-get-appropriate-declaration-regexp) nil t)
      (when (and (match-beginning 0)
                 (= initial-line-number
                    (google-line-number-at-pos (match-beginning 0)))
                 (not (string= (match-string 1) "catch")))
        (let ((match (match-string 1)))
          (set-text-properties 0 (length match) nil match)
          match)))))

(defun google-get-enclosing-function ()
  "In the current buffer, at the current point, get the function name
of the function enclosing the current point."

  ;; The match may not be enclosing but actually on the same line,
  ;; check this out first.
  (save-excursion
    (or (google-get-function-at-same-line)
        (progn
          (search-backward-regexp
           (google-get-appropriate-definition-regexp) nil t)
          ;; Sometimes, the match starts at invalid position 0.
          (let ((match
                 (or (if (eq 0 (match-beginning 1)) "" (match-string 1)) "")))
            ;; Deal with the case of catch statements, which look just like
            ;; method definitions.
            (if (string= match "catch")
                (progn
                  ;; Go to the beginning of the line.
                  (beginning-of-line)
                  (google-get-enclosing-function))
              (set-text-properties 0 (length match) nil match)
              match))))))

(defun google-get-calling-identifier-from-match (match)
  "From a MATCH returned by a gtag server call, get a readable
identifier that should be informative to the user"
  (save-window-excursion
    (save-excursion
      (let ((filename (google-get-path-from-match match))
            (buffer (get-buffer-create " *gtags calltree temp*")))
        (if filename
            (progn
              (set-buffer buffer)
              (erase-buffer)
              (insert-file-contents filename)
              (goto-line (tagrecord-lineno match))
              (google-get-enclosing-identifier filename))
          "unknown")))))

(defun google-get-calling-function-from-match (match)
  "From a MATCH returned by a gtag server call, figure out the
 calling function"
  (save-window-excursion
    (save-excursion
      (let ((buffer (get-buffer-create " *gtags calltree temp*")))
        ;; We don't want to stop just because we couldn't open one file.
        (condition-case nil
            (google-visit-tagrecord match nil nil buffer)
          (error nil))
        ;; This doesn't necessarily stay as the current buffer, so
        ;; reselect it.
        (set-buffer buffer)
        (google-get-enclosing-function)))))

;;******************************************************************************
;; Filenames

(defvar google-default-root "/home/build/google3/"
  "The default location to search for files that aren't checked out
      in your source root." )

(defvar google-source-root ""
  "The root of your source tree client.")

;;; Deduce the current source root from the current file.
(defun find-google-source-root (buffername)
  google-source-root)

(defun google-filename-after-first-slash (filename)
  "Given './FILENAME' or 'FILENAME', return 'FILENAME'"
  (if (string-match "\\(\\./\\)?\\(.*\\)" filename)
      (match-string 2 filename)
    ""))

(defun google-remove-source-prefix (pathname)
  "Hook for stripping common prefix to all source paths. For example, if all
checked out source code lives under 
some-dir-name/source-root/path-to-code-file, then this function should
strip off source-root/ and everything else before it and return 
path-to-code-file which would be the relative path of the file from source
root"
  pathname)

(defun google-get-current-relative-path ()
  "Figures out what path to send to the GTags server for a
google-* command.  Returns the buffer filename with source root
stripped, or nil if no file is being visited or the file path
doesn't contain source root."
  (and buffer-file-name
       (google-remove-source-prefix buffer-file-name)))

(defun google-buffer-file-extension (buffer)
  "Return the filename extension of the file BUFFER is visiting, or
nil if the buffer is not visiting a file."
  (let ((filename (buffer-file-name buffer)))
    (and filename
         (file-name-extension filename))))

;;******************************************************************************
;; Genfiles

(defvar google-filename-generators nil
  "Contains a list of functions that can take a filename and determine if the 
file is a generated file. If it is a generated file, the functions should return
a list of potential source filenames (in fullpath). Note that these are 'potential'
source filenames. It is acceptable if the source filename does not point to an 
actual file. We check for file existence in google-select-from-file. If a function
cannot determine that the filename indicates a generated file, it should return an
empty list. The results from all the functions will be appended together.")

(defun google-generate-potential-filenames-helper (filename generators result)
  "Loops through all the filename generators recursively, and call each to 
generate possible source filename and aggregate the results in a list"
  (if (null generators) 
      result 
    (google-generate-potential-filenames-helper filename
						(cdr generators) 
						(append result 
							(funcall (car generators) 
								 filename)))))

(defun google-generate-potential-filenames (filename default-filename)
  "Given a filename, determines if the file is generated. Returns a list of 
filenames that might be the source file plus the original filename and 
default-filename"
  (append (google-generate-potential-filenames-helper filename 
						      google-filename-generators 
						      nil) 
	  (list filename default-filename)))

;;******************************************************************************
;; Interactive

(defun google-prompt-tag (string &optional language)
  "Prompt the user for a tag using STRING as a prompt.  Returns
the selected tag as a string, or defaults to tag at point if
nothing is typed."
  (let ((original-buffer (current-buffer)))
  ;; gnu-emacs-21's completing-read demands a symbol for the
  ;; completion function, not a lambda, so we bind it here with
  ;; flet.  lexical-let doesn't seem to work here.
  (flet ((completion-function (string predicate operation-type)
            (google-completion-collection-function original-buffer
                                                   string
                                                   predicate
                                                   operation-type
                                                   language)))
    (let* ((default (google-tag-under-cursor))
           (spec (completing-read
                  (if default (format "%s(default %s) " string default)
                    string)
                  #'completion-function
                  nil
                  nil
                  nil
                  'google-find-tag-history
                  default)))
      (if (equal spec "")
          (error "No tag supplied.")
        spec)))))

(defun google-prompt-language ()
  "Prompts the user for a language and returns it as a
string.  Defaults to the language of the current buffer (guessed
from the major mode)."
  (let* ((default-language-entry
           (or (google-mode-to-language
                (car (google-get-buffer-context (current-buffer))))
               (google-mode-to-language google-tags-default-mode)))
         (default-language
           (and default-language-entry (car default-language-entry)))
         (spec
          (completing-read (if (not (equal default-language ""))
                               (format "Language (default %s): "
                                       default-language)
                             "Language: ")
                           google-tags-language-options
                           nil
                           t)))
    (or (and (not (equal spec ""))
             spec)
        default-language
        (error "No language selected."))))

(defun google-tag-under-cursor ()
  "Locate the current identifier that the cursor is pointed at, and
present it as the default, stripping out font information and other
such garbage."
  (save-excursion
    (while (looking-at "\\sw\\|\\s_")
      (forward-char 1))
    (if (or (re-search-backward "\\sw\\|\\s_"
                                (save-excursion (beginning-of-line) (point))
                                t)
            (re-search-forward "\\(\\sw\\|\\s_\\)+"
                               (save-excursion (end-of-line) (point))
                               t))
        (progn (goto-char (match-end 0))
               (buffer-substring-no-properties (point)
                                 (progn (forward-sexp -1)
                                        (while (looking-at "\\s'")
                                          (forward-char 1))
                                        (point))))
      nil)))

(defun google-do-something-with-tag-under-point (if-gtags-property
                                                 if-normal-text)
  (let ((tagrecord (get-text-property (point) 'gtag)))
    (if tagrecord
        (funcall if-gtags-property tagrecord)
      (funcall if-normal-text (google-tag-under-cursor)))))

(defun google-do-something-with-tag-name-under-point (what-to-do)
  (google-do-something-with-tag-under-point
   (lambda (tagrecord)
     (funcall what-to-do
              (tagrecord-tag tagrecord)))
   what-to-do))

(defcustom google-show-tag-location-in-separate-frame nil
  "If turned on, \\[google-show-tag-locations-under-point] will pop up a new
frame and show the selected tag. If the file of the selected tag is already
visible in some frame, then re-use that frame and try to raise it.")

(defun google-show-tag-locations-under-point ()
  "Immediately jump to tag location under the point.  If the text has
a `gtag' property, jump to that specific location rather than looking
the tag up.  May create a new frame if the variable
`google-show-tag-location-in-separate-frame' is set."
  (interactive)
  (google-do-something-with-tag-under-point
   '(lambda (tag) (google-visit-tagrecord tag t nil nil 'show-tag-location))
   'google-show-tag-locations))

(defun google-show-callers-under-point ()
  "Immediately show the tag locations under the point."
  (interactive)
  (google-do-something-with-tag-name-under-point 'google-show-callers))

(defalias
  'google-jump-to-tag-under-point
  'google-show-tag-locations-under-point)

(defalias 'google-find-tag 'google-show-tag-locations)

(defun google-visit-javadoc-under-point ()
  "In GTAGS mode, visit the Javadoc for tag represented by the line."
  (interactive)
  (google-do-something-with-tag-under-point 'google-visit-javadoc
                                            'google-javadoc))

(defun google-show-tag-locations (tagname &optional language)
  "Queries the tag server for a list of exact matches for
TAGNAME.  Presents a buffer showing the results.  If there is only
one match, then jump directly to the match.  Optionally specify
LANGUAGE to check (if omitted, the major mode and file extension
are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (and current-prefix-arg
                                    (google-prompt-language))))
                 (list (google-prompt-tag "Tag: " language)
                       language)))
  (google-get-tags-and-show-results 'lookup-tag-exact
                                    `((tag ,tagname)
                                      (language ,language)
                                      (callers nil)
                                      (current-file
                                       ,(google-get-current-relative-path)))))

(defun google-search-definition-snippets (string &optional language)
  "Queries the tag server for definition snippets containing
STRING.  Presents a buffer showing the results.  Optionally specify
LANGUAGE to check (if omitted, the major mode and file extension
are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (and current-prefix-arg
                                    (google-prompt-language))))
                 (list (google-prompt-tag "Search snippets for: " language)
                       language)))
  (google-get-tags-and-show-results 'lookup-tag-snippet-regexp
                                    `((tag ,string)
                                      (language ,language)
                                      (callers nil)
                                      (current-file
                                       ,(google-get-current-relative-path)))
                                    (google-make-gtags-buffer-name string
                                                                   "SNIPPETS")))

(defun google-search-caller-snippets (string &optional language)
  "Queries the tag server for all snippets containing
STRING.  Presents a buffer showing the results.  Optionally specify
LANGUAGE to check (if omitted, the major mode and file extension
are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (and current-prefix-arg
                                    (google-prompt-language))))
                 (list (google-prompt-tag "Search snippets for: " language)
                       language)))
  (google-get-tags-and-show-results 'lookup-tag-snippet-regexp
                                    `((tag ,string)
                                      (language ,language)
                                      (callers t)
                                      (current-file
                                       ,(google-get-current-relative-path)))
                                    (google-make-gtags-buffer-name string
                                                                   "SNIPPETS")
                                    t))

(defun google-show-callers (tagname &optional language)
  "Queries the tag server for lines containing calls to TAGNAME.
Presents a buffer showing the results.  Optionally specify
LANGUAGE to check (if omitted, the major mode and file extension
are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (and current-prefix-arg
                                    (google-prompt-language))))
                 (list (google-prompt-tag "Tag: " language)
                       language)))
  (google-get-tags-and-show-results 'lookup-tag-exact
                                    `((tag ,tagname)
                                      (language ,language)
                                      (callers t)
                                      (current-file
                                       ,(google-get-current-relative-path)))
                                    (google-make-gtags-buffer-name tagname
                                                                   "CALLERS")
                                    t))

(defun google-show-tag-locations-regexp (tagname &optional language)
  "Queries the tag server for tags beginning with the regexp
TAGNAME.  Presents a buffer showing the results.  Optionally
specify LANGUAGE to check (if omitted, the major mode and file
extension are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (and current-prefix-arg
                                    (google-prompt-language))))
                 (list (google-prompt-tag "Regexp: " language)
                       language)))
  (google-get-tags-and-show-results 'lookup-tag-prefix-regexp
                                    `((tag ,tagname)
                                      (language ,language)
                                      (callers nil)
                                      (current-file
                                       ,(google-get-current-relative-path)))))

(defalias 'google-show-matching-tags 'google-show-tag-locations-regexp)

(defun google-list-tags (filename)
  "List all the tags in FILENAME, defaulting to the one for the
current buffer."
  (interactive "fFilename: ")
  (let* ((pathname (google-remove-source-prefix filename))
         (filename-only (file-name-nondirectory filename))
         (matches (google-get-tags 'lookup-tags-in-file
                                   `((file ,pathname))
                                   :ranked-p t
                                   :error-if-no-results-p t))
         (uniques (sort* (google-remove-association-dups matches)
                         #'string-lessp
                         :key #'tagrecord-tag)))
    (google-tags-show-results uniques
                              (google-make-gtags-buffer-name filename-only))))

(defun google-javadoc (tagname)
  "Jump to the first match for TAGNAME in Google's Javadocs.  Find
the package using Gtags."
  (interactive (list (google-prompt-tag "Javadoc: " 'java)))
  (google-visit-javadoc
   (car
    (google-get-tags 'lookup-tag-exact
                     `((tag ,tagname)
                       (callers nil)
                       (current-file ,(google-get-current-relative-path)))
                     :ranked-p t
                     :error-if-no-results-p t))))

(defun google-visit-tagrecord (tagrecord
                               &optional
                               push
                               display-buffer
                               buffer
			       show-tag-location)
  "Given a tag record, jump to the line/file.  
If PUSH is non-nil then push the current location on a stack.  
If DISPLAY-BUFFER is non-nil then display the result in gdoc-display-window.  
If BUFFER is non-nil then we replace the contents of that buffer with the
file contents (which is good for processing lots of tags in succession).  
If SHOW-TAG-LOCATION is non-nil then possibly pop up a new frame.  
See the variable `google-show-tag-location-in-separate-frame'.  
This function is called from various functions in this file,
as well as gdoc-mode.el
The optional arguments are used to control desired functionality
as seen from each individual caller."
  (let* ((tagname (tagrecord-tag tagrecord))
         (filename (tagrecord-filename tagrecord))
         (snippet (tagrecord-snippet tagrecord))
         (lineno (tagrecord-lineno tagrecord))
         (offset (tagrecord-offset tagrecord))
         (etagrecord (cons snippet (cons lineno offset)))
         (actual-filename
          (expand-file-name
           (concat (file-truename (find-google-source-root default-directory))
                   filename)))
         (default-filename (expand-file-name
                            (concat (file-truename google-default-root)
                                    filename)))
         (selected-file (google-select-from-files
                         (google-generate-potential-filenames
                          actual-filename
                          default-filename)))
	 ;; If selected-file is neither actual-filename nor default-filename
	 ;; then it must be a generated file
         (generated-file (not (or 
			       (equal actual-filename selected-file) 
			       (equal default-filename selected-file)))))
    (if push (google-push-tag))  ; Save the tag so we can come back.
    (save-selected-window
      (if display-buffer
          (select-window gdoc-display-window))
      (if selected-file
          (progn
            (if buffer
                (save-excursion
                  (set-buffer buffer)
                  (erase-buffer)
                  (insert-file-contents selected-file)
                  (goto-char 1)
                  (if generated-file
                      (search-forward tagname)
                    (goto-line lineno)))
	      (if (and window-system
		       show-tag-location
		       google-show-tag-location-in-separate-frame)
		  (google-find-file-other-frame selected-file)
		(find-file selected-file))
	      (if (not generated-file)
		  (etags-goto-tag-location etagrecord)
		;; Generated file.  Just do a search-forward.
		(goto-char (point-min))
		(search-forward tagname))))
        (error "Can't find file any more.")))
    (if (and selected-file  ; Had to put this outside save-selected-window.
	     window-system
	     show-tag-location
	     google-show-tag-location-in-separate-frame)
      (google-raise-frame-of-file selected-file))))

(defun google-raise-frame-of-file (filename)
  "Raise the frame displaying FILENAME."
  (interactive "fFilename :")
  (if (google-xemacs)
      (xemacs-raise-frame-of-file filename)
    (gnuemacs-raise-frame-of-file filename)))

;; In xemacs raise-frame works if we have done select-window first.
(defun xemacs-raise-frame-of-file (filename)
  "Raise the frame displaying FILENAME."
  (interactive "fFilename :")
  (let* ((buffer  (get-file-buffer filename))
	 (window  (get-buffer-window buffer t))
	 (frame   (window-frame window)))
    (save-window-excursion
      (select-window window)
      (raise-frame frame))))

;; This is basically the same as save-window-excursion,
;; except that we save current-frame information, rather than window info.
(defmacro gnuemacs-save-frame-excursion (&rest body)
  "Execute body, preserving selected frame and mouse position."
  (let ((current-frame  (gensym 'current-frame))
	(current-mousep (gensym 'current-mousep)))
    `(let ((,current-frame (selected-frame))
	   (,current-mousep (mouse-position)))
       (unwind-protect
	   (progn ,@body)
	 (select-frame-set-input-focus ,current-frame)
	 (raise-frame)
	 (set-mouse-position ,current-frame
			     (car (cdr ,current-mousep))
			     (cdr (cdr ,current-mousep)))))))

;; In gnu emacs raise-frame only works if the desired frame has input focus.
(defun gnuemacs-raise-frame-of-file (filename)
  "Raise the frame displaying FILENAME."
  (interactive "fFilename :")
  (let* ((buffer  (get-file-buffer filename))
	 (window  (get-buffer-window buffer t))
	 (frame   (window-frame window)))
    (gnuemacs-save-frame-excursion
      (select-frame-set-input-focus frame)
      (raise-frame frame))))

(defun google-find-file-other-frame (filename)
  "Edit FILENAME in another frame, reuse existing frame if possible."
  (interactive "fFilename :")
  (if (google-xemacs)
      (xemacs-find-file-other-frame filename)
    (gnuemacs-find-file-other-frame filename)))

(defun xemacs-find-file-other-frame (filename)
  "Edit FILENAME in another frame, reuse existing frame if possible."
  (interactive "fFilename :")
  (let* ((buffer (get-file-buffer filename))
	 (window (if buffer (get-buffer-window buffer t) nil))
	 (frame  (if window (window-frame window) nil)))
    (if frame (display-buffer buffer t frame)
      (find-file-other-frame filename))
    ;; Must select-window for etags-goto-tag-location to work properly.
    ;; This is executed within save-selected-window anyways.
    (if window (select-window window))))

(defun gnuemacs-find-file-other-frame (filename)
  "Edit FILENAME in another frame, reuse existing frame if possible."
  (interactive "fFilename :")
  (let ((display-buffer-reuse-frames t))
    (find-file-other-frame filename)))

;;******************************************************************************
;; Utility Functions

(defun google-select-from-files (filelist)
  "Return a file that exist from this file, in order."
  (find-if '(lambda (x) (file-exists-p x)) filelist))

(defun google-rename-buffer (old-name new-name)
  (save-excursion
    (set-buffer old-name)
    (rename-buffer new-name)))

(defun google-longest-common-prefix (string1 string2)
  (do ((index 0 (+ index 1)))
      ((or (= index (length string1))
           (= index (length string2))
           (not (= (aref string1 index)
                   (aref string2 index))))
       (substring string1 0 index))))

(defun google-trim-leading-whitespace (string)
  "Return STRING without leading whitespace."
  (if (string= string "")
      string
    (save-match-data
      (if (string-match "^\\s-+" string)
          (substring string (match-end 0))
        string))))

(defun google-trim-trailing-whitespace (string)
  "Return STRING without trailing whitespace."
  (if (string= string "")
      string
    (save-match-data
      (if (string-match "\\s-+$" string)
          (substring string 0 (match-beginning 0))
        string))))

(defun google-trim-whitespace (string)
  "Return STRING without leading or trailing whitespace."
  (google-trim-trailing-whitespace
   (google-trim-leading-whitespace string)))

(defun google-make-gtags-buffer-name (about &optional prefix)
  (format "*%s: %s*"
          (or prefix "TAGS")
          about))

;;******************************************************************************
;; Emacs Modes

;;;;;;;;;;;;;;;;;;;
;; Browse-tags mode

(define-derived-mode google-browse-tags-mode fundamental-mode
  "GTAGS"
  "Major mode for browsing Google tags.
\\[google-visit-javadoc-under-point] to jump Javadoc for a particular tag
\\[google-show-tag-locations-under-point] to jump to a particular tag
\\[google-narrow-by-filename] to perform a narrowing
\\[google-show-all-results] to show all tags
\\[delete-window] to quit browsing")

;; Mode functions

(defalias 'google-visit-tag-under-point 'google-show-tag-locations-under-point)

(defun google-xemacs-visit-tag-pointed-by-mouse ()
 "Weird hack to let Xemacs work properly with the mouse."
  (interactive)
  (mouse-set-point current-mouse-event)
  (google-show-tag-locations-under-point))

(defun google-narrow-by-filename (regexp)
  "Narrow the view to all files matching the provided regular expression"
  (interactive "sNarrow to filenames matching: ")
  (if (not (google-xemacs)) ; Xemacs and GNU emacs are completely different.
      (gnuemacs-narrow-by-filename regexp)
    (xemacs-narrow-by-filename regexp)))

(defun gnuemacs-narrow-by-filename (regexp)
  (dolist (overlay (overlays-in (point-min) (point-max)))
    (let* ((start-prop (overlay-start overlay))
           (tagrecord (get-text-property start-prop 'gtag))
           (filename (tagrecord-filename tagrecord)))
      (if (not (string-match regexp filename))
          (overlay-put overlay 'invisible t)))))

(defun xemacs-narrow-by-filename (regexp)
  (dolist (extent (extent-list))
    (let* ((startpos (extent-start-position extent))
           (endpos (extent-end-position extent))
           (tagrecord (get-text-property startpos 'gtag)))
      (if tagrecord
          (let ((filename (tagrecord-filename tagrecord)))
            (if (not (string-match regexp filename))
                (set-extent-property extent 'invisible t)))))))

(defun google-show-all-results ()
  "Undo the effects of a google-narrow-by-filename."
  (interactive)
  (if (not (google-xemacs))
      (gnuemacs-show-all-overlays)
    (xemacs-show-all-overlays)))

(defun gnuemacs-show-all-overlays ()
  (dolist (overlay (overlays-in (point-min) (point-max)))
    (overlay-put overlay 'invisible nil)))

(defun xemacs-show-all-overlays ()
  (dolist (extent (extent-list))
    (set-extent-property extent 'invisible nil)))

;; Mode shortcuts
(define-key google-browse-tags-mode-map
  "\C-j"
  'google-visit-javadoc-under-point)

(define-key google-browse-tags-mode-map
  "\C-m"
  'google-show-tag-locations-under-point)

(define-key google-browse-tags-mode-map
  "a"
  'google-show-all-results)

(define-key google-browse-tags-mode-map
  "n"
  'google-narrow-by-filename)

(define-key google-browse-tags-mode-map
  "q"
  'delete-window)

(define-key google-browse-tags-mode-map
  "h"
  'describe-mode)

(if (google-xemacs)
    (define-key google-browse-tags-mode-map
      [button1]
      'google-xemacs-visit-tag-pointed-by-mouse)
  (define-key google-browse-tags-mode-map
    [mouse-1]
    'google-show-tag-locations-under-point))

(provide 'gtags)

