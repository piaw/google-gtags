;;; Emacs Lisp Code for handling google tags
;;; Authors: Piaw Na, Arthur A. Gleckler, Leandro Groisman, Phil Sung
;;; Copyright Google Inc. 2004-2007

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
(defun gtags-xemacs()
  (string-match "XEmacs" emacs-version))

(if (gtags-xemacs)
    (progn
      (load "/home/build/public/eng/elisp/xemacs/etags")
      (require 'wid-browse)))

(defconst gtags-client-version 1
  "Version number for this (emacs) gtags client.")

(defconst gtags-protocol-version 2
  "Version number for the protocol that this client speaks.")

(require 'etags)
(require 'cl)
(require 'google-emacs-utilities)
(require 'tree-widget)


;;;;****************************************************************************
;;;; Basic Configuration

(defvar gtags-language-hosts
  '(("c++" . (("gtags.example.com" . 2223)))
    ("java" . (("gtags.example.com" . 2224)))
    ("python" . (("gtags.example.com" . 2225))))
  "Each language is paired with a list of hostname, port pairs. To set up
multiple GTags servers, add additional hostname, port pairs and set
gtags-multi-servers to true")

(defvar gtags-call-graph-hosts
  '(("c++" . (("gtags.example.com" . 2223)))
    ("java" . (("gtags.example.com" . 2224)))
    ("python" . (("gtags.example.com" . 2225))))
  "Similar to gtags-language-hosts, but these are for call graph
servers")

(defvar gtags-language-options
  '(("c++" c++-mode)
    ("java" java-mode)
    ("python" python-mode))
  "Each language is identified by a string which can either be entered
interactively or passed as an argument to any of the gtags-*
functions that expect them.  Maps each string to a mode.")

(defvar gtags-corpus nil
  "*The corpus name specifies which set of servers to use.
Each set of servers might handle a different body of code, e.g. a
different major project.  The corpus name is passed to
`gtags-make-gtags-servers-alist', which can use it when
constructing server names in order to pick which set of servers
to use.  The implementation here ignores it, but it is made
available for customization.")

(defvar gtags-modes-to-languages
  (cons '(jde-mode . "java")
        (mapcar (lambda (association)
                  (cons (cadr association)
                        (car association)))
                gtags-language-options)))

(defvar gtags-extensions-to-languages
  '()
  "an alist mapping each file extension to the corresponding
language")

(defvar gtags-default-mode 'c++-mode
  "Mode for the language to use to look up the Gtags server when
the buffer and file extension don't map to a Gtags server.  Must
be one of the symbols on the right-hand side of alist entries in
`gtags-language-options', e.g. c++-mode, java-mode, or
python-mode.")

(defvar gtags-ranking-methods
  '((include-distance) (directory-distance) (popularity))
  "A list specifying how the tags results should be ranked.
The results will be sorted by the corresponding fields. The list
may contain some or all of the following, in any order:

  (include-distance) : Files included directly or indirectly from
     the current file will be ranked higher than those that are not.
  (directory-distance) : Files closer to the current file in the
     directory tree will be ranked higher.
  (popularity) : Files referenced by many other files will be ranked
     higher.")

(defconst gtags-sleep-time-between-server-polls 0.01)
(defconst gtags-query-connection-timeout 1000) ;; 10 seconds
(defconst gtags-ping-connection-timeout 200)   ;; 2 seconds


;;;;****************************************************************************
;;;; Debug functions

(defvar gtags-print-trace nil)

(defun gtags-pri (text)
  (if gtags-print-trace
      (princ text (get-buffer-create "**dbg**"))))

(defun gtags-pril (text)
  (when gtags-print-trace
    (princ text (get-buffer-create "**dbg**"))
    (terpri (get-buffer-create "**dbg**"))))

(defun gtags-get-buffer-context (buffer)
  "given buffer, get context (mode and file extension)"
  (let* ((mode (save-excursion (set-buffer buffer) major-mode))
         (file-extension (gtags-buffer-file-extension buffer)))
    (cons mode file-extension)))


;;;;****************************************************************************
;;;; Data definition

(defun gtags-alist-get (key lst)
  "Returns the value associated with KEY in a list that looks
like ((key value) ...), or nil if nothing matches KEY."
  (let ((matching-entry (assoc key lst)))
    (and matching-entry (cadr matching-entry))))

(defun gtags-alist-error-message (lst)
  "Return message-value from an alist in the form of
((error ((message message-value) ...)))"""
  (gtags-alist-get 'message (gtags-alist-get 'error lst)))

(defun gtags-response-signature (response)
  "Extracts (server-start-time sequence-number) from a server
response (this is a unique identifier for the request)."
  (let ((server-start-time (gtags-alist-get 'server-start-time response))
        (sequence-number (gtags-alist-get 'sequence-number response)))
    `(,@server-start-time ,sequence-number)))

(defun gtags-response-value (response)
  "Extracts the 'value' from a server response.  This is a list of
tagrecords, for commands that return tags."
  (gtags-alist-get 'value response))

;; A tagrecord stores the information associated with a single
;; matching tag.
(defmacro deftagrecord (name)
  (let ((tagrecord-name
         (intern (concat "gtags-tagrecord-" (symbol-name name)))))
    `(defun ,tagrecord-name (tagrecord)
       (gtags-alist-get ',name tagrecord))))

(deftagrecord directory-distance)
(deftagrecord filename)
(deftagrecord lineno)
(deftagrecord offset)
(deftagrecord snippet)
(deftagrecord tag)


;;;;****************************************************************************
;;;; Tag history stack

(defvar gtags-history nil
  "a stack of visited locations")

(defun gtags-pop-tag ()
  "Pop back up to the previous tag."
  (interactive)
  (if gtags-history
      (let* ((gtags-tag-history-entry (car gtags-history))
             (buffname (car gtags-tag-history-entry))
             (pt (cdr gtags-tag-history-entry)))
        (setq gtags-history (cdr gtags-history))
        (switch-to-buffer buffname)
        (goto-char pt))
    (error "No tag to pop!")))

(defun gtags-push-tag ()
  "Push the current location onto the tag stack."
  (interactive)
  (push-mark)
  (let ((currloc (cons (buffer-name) (point))))
    (setq gtags-history (cons currloc gtags-history))))


;;;;****************************************************************************
;;;; Tag navigation

(defalias 'gtags-first-tag 'gtags-feeling-lucky)

(defvar gtags-next-tags nil
  "List of GTags matches remaining for use with `gtags-next-tag'.
Includes the current tag as its first element.")

(defvar gtags-previous-tags nil
  "List of GTags matches previously visited with `gtags-next-tag'.")

(defun gtags-feeling-lucky (tagname &optional language)
  "Jump to the first exact match for TAGNAME.
Optionally specify LANGUAGE to check (if omitted, the major mode
and file extension are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (gtags-language current-prefix-arg)))
                 (list (gtags-prompt-tag "Tag: " language)
                       language)))
  (let ((matches (gtags-get-tags 'lookup-tag-exact
                                  `((tag ,tagname)
                                    (language
				     ,(gtags-guess-language-if-nil
				       language))
                                    (callers nil)
                                    (current-file ,buffer-file-name)
                                    (ranking-methods
                                     ,@gtags-ranking-methods))
                                  :ranked-p t
                                  :error-if-no-results-p t)))
    (setq gtags-next-tags matches)
    (setq gtags-previous-tags nil)
    (gtags-visit-tagrecord (car matches) t)))

(defun gtags-next-tag ()
  "After `gtags-feeling-lucky', jump to the next match."
  (interactive)
  (if (cdr gtags-next-tags)
      (let ((this-match (car gtags-next-tags)))
        (setq gtags-next-tags (cdr gtags-next-tags))
        (setq gtags-previous-tags (cons this-match gtags-previous-tags))
        (gtags-visit-tagrecord (car gtags-next-tags)))
    (error "No more matches found!")))

(defun gtags-previous-tag ()
  "After `gtags-next-tag', returns to the previous match."
  (interactive)
  (if gtags-previous-tags
      (let ((previous-match (car gtags-previous-tags)))
        (setq gtags-next-tags (cons previous-match gtags-next-tags))
        (setq gtags-previous-tags (cdr gtags-previous-tags))
        (gtags-visit-tagrecord (car gtags-next-tags)))
    (error "No more matches found!")))


;;;;****************************************************************************
;;;; Tag completion

(defun gtags-completion-collection-function (buffer
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
     (let ((completions (gtags-all-matching-completions string
                                                         predicate
                                                         buffer
                                                         language)))
       (and completions
            (let ((first-completion (car completions)))
              (or (and (null (cdr completions))
                       (string-equal string first-completion))
                  (gtags-longest-common-prefix first-completion
                                                (car (last completions))))))))
    ;; t specifies all-completions.  Return a list of all possible
    ;; completions of the specified string.
    ((t) (gtags-all-matching-completions string predicate buffer language))
    ;; lambda specifies a test for an exact match.  Return t if the
    ;; specified string is an exact match for some possibility; nil
    ;; otherwise.
    ((lambda) (gtags-get-tags 'lookup-tag-exact
                               `((tag ,string)
                                 (language
				  ,(gtags-guess-language-if-nil language))
                                 (callers nil)
                                 (current-file ,buffer-file-name)
                                 (ranking-methods
                                  ,@gtags-ranking-methods))
                               :buffer buffer
                               :filter predicate))))

(defun gtags-all-matching-completions (prefix
                                        predicate
                                        &optional
                                        buffer
                                        language)
  "Returns a list of all tags that begin with PREFIX.  Optionally
override the language to search with LANGUAGE."
  (remove-duplicates (gtags-get-tags 'lookup-tag-prefix-regexp
                                      `((tag ,prefix)
                                        (language
					 ,(gtags-guess-language-if-nil
					   language))
                                        (callers nil)
                                        (current-file ,buffer-file-name)
                                        (ranking-methods
                                         ,@gtags-ranking-methods))
                                      :buffer (or buffer (current-buffer))
                                      :filter predicate
                                      :map #'gtags-tagrecord-tag)
                     :test #'equal))

(defun gtags-complete-tag ()
  "Perform GTags completion on the text around point."
  (interactive)
  (let ((tag (gtags-tag-under-cursor)))
    (unless tag
      (error "Nothing to complete"))
    (let* ((start (progn (search-backward tag) (point)))
           (end (progn (forward-char (length tag)) (point)))
           (completions (gtags-all-matching-completions tag nil)))
      (cond ((null completions)
             (message "Can't find any completions for \"%s\"" tag))
            ((null (cdr completions))
             (if (string-equal tag (car completions))
                 (message "No more completions.")
               (delete-region start end)
               (insert (car completions))))
            (t
             (let ((longest-common-prefix
                    (reduce #'gtags-longest-common-prefix completions)))
               (if (string-equal tag longest-common-prefix)
                   (progn
                     (message "Making completion list...")
                     (with-output-to-temp-buffer "*Completions*"
                       (display-completion-list completions))
                     (message "Making completion list...%s" "done"))
                 (delete-region start end)
                 (insert longest-common-prefix))))))))


;;;;****************************************************************************
;;;; Main functions

(defun gtags-get-tags-and-show-results (command-type
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
  (gtags-show-results
   (gtags-get-tags command-type
                    parameters
                    :caller-p caller-p
                    :ranked-p t
                    :error-if-no-results-p t)
   (or buffer-name
       ;; First try to name buffer using tag, then fall back to file
       (gtags-make-gtags-buffer-name
        (or (gtags-alist-get 'tag parameters)
            (gtags-alist-get 'file parameters))))))

(defun* gtags-get-tags (command-type
                         parameters
                         &key
                         caller-p
                         ranked-p
                         buffer
                         error-if-no-results-p
                         filter
                         map)
  (gtags-pril "gtags-get-tags")
  (let* ((buffer (or buffer (current-buffer)))
         (matches (gtags-get-tags-from-server command-type
                                               parameters
                                               caller-p
                                               buffer))
         (matches-filtered (if filter
                               (google-filter filter matches)
                             matches))
         (matches-ranked (if ranked-p
                             (gtags-rank-tags matches-filtered
                                               buffer)
                           matches-filtered))
         (matches-mapped (if map
                             (mapcar map matches-ranked)
                           matches-ranked))
	 (error-message (gtags-alist-error-message matches-mapped)))
    (cond ((not (null error-message))
	   (error error-message))
	  ((and error-if-no-results-p
		(null matches-mapped))
	   (error "No tag found!"))
	  (t matches-mapped))))

(defvar gtags-output-options
  '(("standard" gtags-show-tags-option-window-standard)
    ("single-line" gtags-show-tags-option-window-grep-style)
    ("single-line-grouped" gtags-show-tags-option-window-grouped)))

(defvar gtags-output-mode 'standard)

(defun gtags-output-mode ()
  "Set the GTags result buffer format interactively."
  (interactive)
  (let ((mode (completing-read "Choose Mode (standard): "
                               gtags-output-options
                               nil
                               t
                               nil
                               nil
                               "standard")))
    (setq gtags-output-mode (intern mode))))


(defun gtags-show-tags-option-window (matches
                                       buffer-name
                                       current-directory)
  (select-window
   (gtags-show-tags-option-window-inner
    matches
    (get-buffer-create buffer-name)
    current-directory
    #'display-buffer)))

(defun gtags-show-tags-option-window-inner (matches
                                             buffer
                                             current-directory
                                             make-window)
  (let ((output-formatter-entry
         (assoc (symbol-name gtags-output-mode)
                gtags-output-options)))
    (cond (output-formatter-entry
           (set-buffer buffer)
           (toggle-read-only 0)
           (erase-buffer)
           (gtags-browse-tags-mode)
           (set (make-local-variable 'truncate-lines) t)
           (let ((window (funcall make-window buffer)))
             (save-excursion
               (funcall (cadr output-formatter-entry)
                        matches
                        buffer
                        current-directory
                        window))
             (set (make-local-variable 'default-directory) current-directory)
             (toggle-read-only 1)
             (gtags-show-warning-if-multi-server-is-too-slow)
             window))
          (t (error "No such output mode %s." gtags-output-mode)))))

(defun gtags-show-warning-if-multi-server-is-too-slow ()
  (when gtags-warning-multi-server-slow
    (message (concat "Multi server mode too slow?  Consider "
                     "using M-x gtags-set-single-data-server RET"))
    (setq gtags-warning-multi-server-slow nil)))

(defun gtags-show-tags-option-window-grep-style (matches
                                                  buffer
                                                  current-directory
                                                  window)
  (gtags-show-tags-option-window-single-line-inner
   matches
   buffer
   current-directory
   window
   nil))

(defun gtags-show-tags-option-window-grouped (matches
                                               buffer
                                               current-directory
                                               window)
  (gtags-show-tags-option-window-single-line-inner
   matches
   buffer
   current-directory
   window
   t))

(defun gtags-show-tags-option-window-single-line-inner (matches
                                                         buffer
                                                         current-directory
                                                         window
                                                         grouped-p)
  (let ((last-filename nil))
    (dolist (tagrecord matches)
      (let ((prop-start (point))
            (tag (gtags-tagrecord-tag tagrecord))
            (lineno (gtags-tagrecord-lineno tagrecord))
            (snippet (gtags-trim-whitespace
                      (gtags-tagrecord-snippet tagrecord)))
            (filename (gtags-tagrecord-filename tagrecord)))
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
        (if (gtags-xemacs)
            (gtags-xemacs-mark-extent prop-start (point)))))))

(defun gtags-show-tags-option-window-standard (matches
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
             (tag (gtags-tagrecord-tag tagrecord))
             (snippet (gtags-tagrecord-snippet tagrecord))
             (filename (gtags-tagrecord-filename tagrecord)))
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
        (if (gtags-xemacs)
            (gtags-xemacs-mark-extent prop-start (point) face))))))

(defun gtags-show-results (matches
                                 buffer-name)
  "Given matches, show results."
  (gtags-push-tag)                     ; Save actual position.
  (let ((current-directory default-directory))
    (cond ((or (= (length matches) 1)
               (and gtags-auto-visit-colocated-tags
                    (gtags-matches-colocated-p matches)))
           (gtags-visit-tagrecord (car matches) nil nil))
          (t (gtags-show-tags-option-window matches
                                             buffer-name
                                             current-directory)))))

(defcustom gtags-auto-visit-colocated-tags nil
  "If turned on, whenever all the entries are in the same file, jump
to them."
  :group 'gtags)

(defun gtags-matches-colocated-p (matches)
  "Return t if all the tag matches in MATCHES are in the same file.
Does this by checking if any files don't match the first entry."
  (let ((firstfile (gtags-tagrecord-filename (car matches))))
    (not (find-if (lambda (tagrecord)
                    (not (string= firstfile
                                  (gtags-tagrecord-filename tagrecord))))
                  (cdr matches)))))

(defvar gtags-find-tag-history nil
"History of tags prompted for in Gtags.")

(defvar gtags-javadoc-url-template
  "http://example.com/engineering/javadocs/%s.html"
  "Template for URLs for your Javadocs.
It is in the form of a `format' string, with \"%s\" where the
path to the class should appear in the URL.  This is used to open
a browser window on Java classes found in the Gtags index.")

(defun gtags-visit-javadoc (tagrecord)
  "Visit the Javadoc corresponding to TAGRECORD."
  (let* ((gtags-pathname (gtags-tagrecord-filename tagrecord))
         (javadoc-pathname
          (concat (file-name-directory gtags-pathname)
                  (file-name-nondirectory
                   (file-name-sans-extension gtags-pathname)))))
    (browse-url
     (format gtags-javadoc-url-template javadoc-pathname)
     t)))

(defun gtags-get-tag-file (tag)
  "Returns the file containing the tag"
  (gtags-filename-after-first-slash (gtags-tagrecord-filename tag)))


;;;;****************************************************************************
;;;; Servers

(defvar gtags-multi-servers nil)

(defun gtags-set-multi-server ()
  "Enables multi-server mode for GTags."
  (interactive)
  (setq gtags-multi-servers t)
  (message "Gtags is now in multi-server mode."))

(defun gtags-set-single-server ()
  "Enables single-server mode for GTags."
  (interactive)
  (setq gtags-multi-servers nil)
  (message (concat
            "Gtags is now in single server mode.  To switch back, "
            "run M-x gtags-set-multi-server.")))

(defun gtags-make-gtags-servers-alist (caller-p corpus language)
  "Given the language and whether call graph is requested, return a list of
servers that can handle the request.  Corpus, which is ignored here,
can be used to specify which of several sets of servers to use.
Each set of servers might handle a different body of code, e.g. a
different major project."
  (cdr (assoc language (if caller-p
			   gtags-call-graph-hosts
			 gtags-language-hosts))))

(defun gtags-mode-to-language (mode)
  (rassoc* mode gtags-language-options :key #'car))


;;;;****************************************************************************
;;;; Connection management

(defconst gtags-connection-attempts 3)

(defconst gtags-server-buffer-name "*TAGS server*"
  "buffer used for communication with gtags server")

(defun gtags-get-tags-from-server (command-type
                                    parameters
                                    caller-p
                                    buffer)
  "Choose a server and return the tags.
See `gtags-get-tags-and-show-results' for a description of the parameters."
  (gtags-choose-server-and-get-tags command-type
                                     gtags-corpus
                                     parameters
                                     caller-p
                                     (gtags-get-buffer-context buffer)
                                     gtags-connection-attempts))

(defun gtags-cannot-connect ()
  (error "Cannot connect to server.  Try again in a few minutes."))

(defun gtags-choose-server-and-get-tags (command-type
                                          corpus
                                          parameters
                                          caller-p
                                          context
                                          attempts)
  "Choose best server, issue a request, and return only the
matching-tags list."
  (flet ((issue-command ()
           (gtags-issue-command
            command-type
            corpus
            parameters
            (gtags-find-host-port-pair caller-p
                                             context
                                             corpus
                                             parameters))))
    (do ((response (issue-command) (issue-command))
       (attempts-left attempts
                      (1- attempts-left)))
      ((or (listp response) (= attempts-left 0))
       (if (listp response)
           (gtags-response-value response)
	 (gtags-cannot-connect)))
    (gtags-pri "ATTEMPTs left: ")
    (gtags-pri attempts)
    (gtags-pri " -- ")
    (gtags-pril response)
    (memoize-forget 'gtags-find-host-port-pair))))

(defvar gtags-choose-server-and-get-tags-memoize-result-count 100)
(defvar gtags-choose-server-and-get-tags-memoize-interval-seconds (* 60 60))

(memoize 'gtags-choose-server-and-get-tags
         gtags-choose-server-and-get-tags-memoize-result-count
         gtags-choose-server-and-get-tags-memoize-interval-seconds)


;;;;****************************************************************************
;;;; Finding server

(defun gtags-find-host-port-pair (caller-p
                                        context
                                        corpus
                                        &optional parameters)
  "Given CONTEXT (mode and file extension), CALLER-P (whether
we're searching for callers), and CORPUS, return the host/port pair
for the fastest server.

If provided, language information in PARAMETERS (list of
name/value pairs as specified by the protocol) may override the
the server that would otherwise be selected based on the
context."
  (gtags-pril "Find host port pair")
  (let ((pairs
         (gtags-find-host-port-pairs caller-p context corpus parameters))
        (selected-pair nil)
        (best-time gtags-starting-best-time)
        (begin-find-host-port-pair (current-time)))
    (if gtags-multi-servers
        (dolist (pair pairs)
          (let* ((before (current-time))
                 (temp-buffer-name "*gtags-server-response-time*")
                 (succeed (gtags-send-string-to-server
                           (with-output-to-string
                             (prin1 (gtags-make-command 'ping corpus '())))
                           temp-buffer-name
                           pair
                           gtags-ping-connection-timeout))
                 (after (current-time))
                 (elapsed-time (gtags-elapsed-time before after)))
            (gtags-pri "Server response: ")
            (if succeed (gtags-pril (gtags-buffer-string temp-buffer-name)))
            (if (and succeed
                     (equal (gtags-response-value
                             (gtags-server-response temp-buffer-name))
                            t)
                     (< elapsed-time best-time))
                (progn
                  (setq selected-pair pair)
                  (setq best-time elapsed-time)
                  (gtags-pri "New pair chosen: ")
                  (gtags-pri selected-pair)
                  (gtags-pri " -- temp ")
                  (gtags-pril elapsed-time))
              (if succeed
                  (progn
                    (gtags-pri elapsed-time)
                    (gtags-pril " too slow"))
                (gtags-pril "failed!")))))
      (setq selected-pair (car pairs)))
    (gtags-pri "Total elapsed time choosing the server")
    (gtags-pril (gtags-elapsed-time begin-find-host-port-pair (current-time)))
    (if (> (gtags-elapsed-time begin-find-host-port-pair (current-time))
           gtags-multi-server-too-slow-threshold)
        (setq gtags-warning-multi-server-slow t))
    selected-pair))

(defconst gtags-starting-best-time 20.0)
(defvar gtags-warning-multi-server-slow nil)
(defvar gtags-multi-server-too-slow-threshold 8)
(defvar gtags-host-port-memoization-result-count 100)
(defvar gtags-host-port-memoization-interval-seconds 6000)

(memoize 'gtags-find-host-port-pair
         gtags-host-port-memoization-result-count
         gtags-host-port-memoization-interval-seconds)

(defun gtags-guess-language (&optional context parameters)
  "Given the context (a pair of a mode and a filename extension) and
the parameters to be sent to the Gtags server, return the best guess
at the programming language being used.  If no context is supplied,
derive one from the current buffer."
  (let ((context (or context (gtags-get-buffer-context (current-buffer)))))
    (or (gtags-alist-get 'language parameters)
        (cdr (assq (car context)
                   gtags-modes-to-languages))
        (let ((file-extension (cdr context)))
          (and file-extension
               (cdr (assoc file-extension
                           gtags-extensions-to-languages))))
        (car (gtags-mode-to-language gtags-default-mode)))))

(defun gtags-guess-language-if-nil (language)
  "If language is nil, guess what programming language we are using based on
current buffer, otherwise return language"
  (or language (gtags-guess-language)))

(defun gtags-find-host-port-pairs (caller-p
                                         context
                                         corpus
                                         &optional parameters)
  "Given CONTEXT (mode and file extension), CALLER-P (whether
we're searching for callers), and CORPUS, return a list of host/port
pairs for all servers that could answer the request.

If provided, language information in PARAMETERS (list of
name/value pairs as specified by the protocol) may override the
the servers that would otherwise be selected."
  (if gtags-use-gtags-mixer
      (list (cons "localhost" gtags-mixer-port))
    (let ((language (gtags-guess-language context parameters)))
      (gtags-make-gtags-servers-alist caller-p corpus language))))

(defun gtags-elapsed-time (start end)
  "Elapsed time between 'current-time's"
  (+ (* (- (car end) (car start)) 65536.0)
     (- (cadr end) (cadr start))
     (/ (- (caddr end) (caddr start)) 1000000.0)))

(defun gtags-server-response (buff)
  "Returns the server response from a buffer, i.e. the first
s-exp in the buffer."
  (save-excursion
    (set-buffer buff)
    (goto-char (point-min))
    (read (current-buffer))))

(defun gtags-buffer-string (buff)
  (save-excursion
    (set-buffer buff)
    (buffer-string)))


;;;;****************************************************************************
;;;; Managing streams

(defun gtags-command-boilerplate ()
  "List of the standard parameters that have to go with every
request."
  (let ((client-type-string (if (gtags-xemacs)
                                "xemacs"
                              "gnu-emacs")))
    `((client-type ,client-type-string)
      (client-version ,gtags-client-version)
      (protocol-version ,gtags-protocol-version))))

(defun gtags-make-command (command-type corpus extra-params)
  "Makes a command with the specified type, corpus, and parameters
(and the boilerplate client/protocol versions) and returns it."
  `(,command-type ,@(gtags-command-boilerplate)
                  (corpus ,corpus)
                  ,@extra-params))

(defun gtags-issue-command (command-type corpus extra-params host-port-pair)
  "Issues a command with the specified type and parameters (and
the boilerplate client/protocol versions) and returns the result."
  (gtags-send-command-to-server (gtags-make-command command-type
                                                      corpus
                                                      extra-params)
                                 host-port-pair))

(defun gtags-send-command-to-server (command host-port-pair)
  "Sends an s-exp to the server and returns the response as a s-exp."
  (if (gtags-send-string-to-server (with-output-to-string
                                      (prin1 command))
                                    gtags-server-buffer-name
                                    host-port-pair
                                    gtags-query-connection-timeout)
      (gtags-server-response gtags-server-buffer-name)
    'error))

(defun gtags-send-string-to-server (command-string
                                     response-buffer-name
                                     host-port-pair
                                     connection-timeout-limit)
  "Send a command (string) to the tag server, then wait for the
response and write it to a buffer.

COMMAND-STRING: string we will send to the server

RESPONSE-BUFFER-NAME: buffer where the server response will be stored

HOST-PORT-PAIR: (host . port) server network address

CONNECTION-TIMEOUT-LIMIT: How many times the server will be polled.  The actual
limit is (connection-timeout-limits * gtags-sleep-time-between-server-polls)
secs.

It returns nil if something goes wrong."
  (gtags-pril "google send tag to server")
  (gtags-pri "buffer: ")
  (gtags-pril response-buffer-name)
  (gtags-pri "host-port: ")
  (gtags-pril host-port-pair)
  (gtags-pri (car host-port-pair))
  (gtags-pri " ")
  (gtags-pril (cdr host-port-pair))
  (gtags-pri "command-string: ")
  (gtags-pril command-string)
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
              (error (gtags-pril "Error gtags-0044")
                     (gtags-pril stream-error-code)
                     nil))))
    (and network-stream ;; Keep going if the stream could be opened
         ((lambda (x) t)
          (condition-case stream-error-code
              (process-send-string network-stream-name
                                   (concat command-string "\r\n"))
            (error (gtags-pril "Error gtags-0045")
                   (gtags-pril stream-error-code)
                   (setq network-stream nil))))
         network-stream ;; Keep going if the string could be sent.
         (gtags-wait-until-stream-is-ready network-stream
                                            connection-timeout-limit)
         (gtags-rename-buffer temp-response-buffer-name
                               response-buffer-name))))

(defun gtags-network-stream-ready (network-stream)
  "The network connection is ready to be read if it is closed"
  (case (process-status network-stream)
    ('open nil)
    ('closed t)))

(defun gtags-wait-until-stream-is-ready (network-stream
                                          connection-timeout-limit)
  "Wait for the connection to be ready
timeout = gtags-sleep-time-between-server-polls * connection-timeout

Return nil in case of timeout"
  (gtags-pril "I will wait for the server")
  (let ((stream-is-ready nil)
        (times-polled 0))
    (while (and (not stream-is-ready) (< times-polled connection-timeout-limit))
      (setq stream-is-ready (gtags-network-stream-ready network-stream))
      (incf times-polled)
      (if (not stream-is-ready)
          (sleep-for gtags-sleep-time-between-server-polls)))
    (gtags-pri "Waiting finished, stream ready?")
    (gtags-pril stream-is-ready)
    stream-is-ready))


;;;;****************************************************************************
;;;; Rank

(defun gtags-rank-tags (matches buf)
  "Given the matches of a query, possibly order them on the basis
of gtags-bias-to-header-files"
  (cond ((and gtags-bias-to-header-files
              (= (length matches) 2)
              (let* ((match1 (car matches))
                     (match2 (cadr matches))
                     (file1 (gtags-tagrecord-filename match1))
                     (file2 (gtags-tagrecord-filename match2)))
                (and (equal (file-name-sans-extension file1)
                            (file-name-sans-extension file2))
                     (or (equal (file-name-extension file1) "h")
                         (equal (file-name-extension file2) "h")))))
         (if (equal (file-name-extension
                     (gtags-tagrecord-filename (car matches)))
                    "h")
             (list (car matches))
           (list (cadr matches))))
        (t matches)))

(defcustom gtags-bias-to-header-files nil
  "If turned on, whenever there are only two entries and there's just a header
file and a .cc file, we ignore the .cc file and jump straight to the header."
  :group 'gtags)


;;;;****************************************************************************
;;;; Tree View

(defun gtags-show-call-tree (tagname)
  "Show the recursive callers of TAGNAME in a tree."
  (interactive (list (gtags-prompt-tag "Tag: ")))
  (let ((orig-buf (current-buffer))
        (tag-identifier (concat (gtags-get-enclosing-class
                                 (buffer-file-name))
                                 "." tagname))
        (buf (get-buffer-create (concat "GTags call graph for "
                                        tagname))))
    (switch-to-buffer buf)
    (erase-buffer)
    (insert "Warning: This gtag-based calltree tends to over-include items.\n")
    (widget-create 'tree-widget
                   :tag tag-identifier
                   :dynargs 'gtags-get-children-from-tree
                   :has-children t
                   :sig (cons orig-buf tagname))
    (use-local-map widget-keymap)
    (widget-setup)))

(defun gtags-calling-function-to-tagname (function-name)
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

(defun gtags-get-children-from-tree (tree)
  (let* ((sig (widget-get tree :sig))
         (tagname (cdr sig)))
    (save-window-excursion
      (save-excursion
        (if (bufferp (car sig))
            (set-buffer (car sig))
          (gtags-visit-tagrecord (car sig) nil nil)
          (find-file (gtags-get-path-from-match (car sig))))
        (message "Getting list of matches for tag %s...." tagname)
        (let* ((matches
                (gtags-get-tags 'lookup-tag-exact
                                 `((tag ,tagname)
                                   (callers t))
                                 :caller-p t
                                 :ranked-p t))
               (count 0))
          (message "Matches retrieved")
          (mapcan
           (lambda (match)
             (let* ((file (gtags-get-path-from-match match))
                    (calling-function
                     (gtags-get-calling-function-from-match match))
                    (display-name
                     (gtags-get-calling-identifier-from-match match))
                    (calling-tagname
                     (gtags-calling-function-to-tagname calling-function)))
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
                                :notify gtags-caller-notify)
                        :dynargs 'gtags-get-children-from-tree
                        :sig (cons match calling-tagname)
                        :has-children t)))))
           matches))))))

(defun gtags-caller-notify (widget child &optional event)
  (gtags-visit-tagrecord (widget-get widget :caller) nil nil))

(defun gtags-get-path-from-match (match)
  (let* ((filename (gtags-tagrecord-filename match))
         (actual-filename
          (expand-file-name
           (concat (gtags-directory-truename
                    (gtags-find-source-root default-directory))
                   filename)))
         (default-filename (expand-file-name
                            (concat
                             (gtags-directory-truename gtags-default-root)
                             filename))))
    (gtags-select-from-files (cons
                               filename (gtags-generate-potential-filenames
                                         actual-filename default-filename)))))

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
  (defvar gtags-c-or-java-function-regexp function-declaration-regexp)
  (defvar gtags-c-or-java-function-definition-regexp
    function-definition-regexp)
  (defvar gtags-python-function-definition-regexp py-function-regexp))

(defun gtags-get-appropriate-definition-regexp ()
  "Get the regexp that is appropriate for the buffer mode (a
python regexp for python mode, java regexp for java mode, etc."

  (if (eq major-mode 'python-mode)
      gtags-python-function-definition-regexp
    gtags-c-or-java-function-definition-regexp))

(defun gtags-get-appropriate-declaration-regexp ()
  "Get the regexp that is appropriate for the buffer mode (a
python regexp for python mode, java regexp for java mode, etc."
  (if (eq major-mode 'python-mode)
      gtags-python-function-definition-regexp
    gtags-c-or-java-function-regexp))

(defun gtags-line-number-at-pos (&optional pos)
  "This is a replicating the functionality of v22 function
`line-number-at-pos'."

  (save-excursion
    (when pos (goto-char pos))
    (+ (count-lines (point-min) (point))
       (if (= (current-column) 0) 1 0))))

(defun gtags-get-enclosing-identifier (filename)
  "In the current buffer, at the current point, get an identifier
such as FooClass.barMethod that identifies the current point"
  (let ((function-name (gtags-get-enclosing-function))
        (simple-identifier "\\([[:alnum:]\.-_]+\\)" ))
    (concat
     (if (string-match (concat simple-identifier "::" simple-identifier)
                       function-name)
         (concat (match-string 1 function-name) "."
                 (match-string 2 function-name))
       (concat (gtags-get-enclosing-class filename) "."
               function-name)) ":"
     (int-to-string (gtags-line-number-at-pos)))))

(defun gtags-get-enclosing-class (filename)
  "Transform the given filename into a classname"
  (condition-case err
      (file-name-sans-extension (file-name-nondirectory filename))
    (error
     (concat "You must be visiting a Java/C++ or Python file"
             "to use this functionality."))))

(defun gtags-get-function-at-same-line ()
  "If there is a function on the same line, then return it, otherwise
return nil"
  (save-excursion
    (let ((initial-line-number (gtags-line-number-at-pos)))
      (search-forward-regexp
       (gtags-get-appropriate-declaration-regexp) nil t)
      (when (and (match-beginning 0)
                 (= initial-line-number
                    (gtags-line-number-at-pos (match-beginning 0)))
                 (not (string= (match-string 1) "catch")))
        (let ((match (match-string 1)))
          (set-text-properties 0 (length match) nil match)
          match)))))

(defun gtags-get-enclosing-function ()
  "In the current buffer, at the current point, get the function name
of the function enclosing the current point."

  ;; The match may not be enclosing but actually on the same line,
  ;; check this out first.
  (save-excursion
    (or (gtags-get-function-at-same-line)
        (progn
          (search-backward-regexp
           (gtags-get-appropriate-definition-regexp) nil t)
          ;; Sometimes, the match starts at invalid position 0.
          (let ((match
                 (or (if (eq 0 (match-beginning 1)) "" (match-string 1)) "")))
            ;; Deal with the case of catch statements, which look just like
            ;; method definitions.
            (if (string= match "catch")
                (progn
                  ;; Go to the beginning of the line.
                  (beginning-of-line)
                  (gtags-get-enclosing-function))
              (set-text-properties 0 (length match) nil match)
              match))))))

(defun gtags-get-calling-identifier-from-match (match)
  "From a MATCH returned by a gtag server call, get a readable
identifier that should be informative to the user"
  (save-window-excursion
    (save-excursion
      (let ((filename (gtags-get-path-from-match match))
            (buffer (get-buffer-create " *gtags calltree temp*")))
        (if filename
            (progn
              (set-buffer buffer)
              (erase-buffer)
              (insert-file-contents filename)
              (goto-line (gtags-tagrecord-lineno match))
              (gtags-get-enclosing-identifier filename))
          "unknown")))))

(defun gtags-get-calling-function-from-match (match)
  "From a MATCH returned by a gtag server call, figure out the
 calling function"
  (save-window-excursion
    (save-excursion
      (let ((buffer (get-buffer-create " *gtags calltree temp*")))
        ;; We don't want to stop just because we couldn't open one file.
        (condition-case nil
            (gtags-visit-tagrecord match nil nil buffer)
          (error nil))
        ;; This doesn't necessarily stay as the current buffer, so
        ;; reselect it.
        (set-buffer buffer)
        (gtags-get-enclosing-function)))))


;;;;****************************************************************************
;;;; Filenames

(defvar gtags-default-root "/tmp/"
  "The default directory to search for files that aren't checked out
in your source root." )

(defvar gtags-source-root ""
  "The root of your source tree, e.g. \"/home/hacker/src1/corpus\".")

;;; Deduce the current source root from the current file.
(defun gtags-find-source-root (buffername)
  gtags-source-root)

(defun gtags-filename-after-first-slash (filename)
  "Given './FILENAME' or 'FILENAME', return 'FILENAME'"
  (if (string-match "\\(\\./\\)?\\(.*\\)" filename)
      (match-string 2 filename)
    ""))

(defun gtags-buffer-file-extension (buffer)
  "Return the filename extension of the file BUFFER is visiting, or
nil if the buffer is not visiting a file."
  (let ((filename (buffer-file-name buffer)))
    (and filename
         (file-name-extension filename))))


;;;;****************************************************************************
;;;; Generated files

(defvar gtags-filename-generators nil
  "Contains a list of functions that can take a filename and
determine if the file is a generated file. If it is a generated
file, the functions should return a list of potential source
filenames (in fullpath). Note that these are 'potential' source
filenames. It is acceptable if the source filename does not point
to an actual file. We check for file existence in
gtags-select-from-file. If a function cannot determine that the
filename indicates a generated file, it should return an empty
list. The results from all the functions will be appended
together.")

(defun gtags-generate-potential-filenames-helper (filename generators result)
  "Loops through all the filename generators recursively, and call each to
generate possible source filename and aggregate the results in a list"
  (if (null generators)
      result
    (gtags-generate-potential-filenames-helper
     filename
     (cdr generators)
     (append result
             (funcall (car generators)
                      filename)))))

(defun gtags-generate-potential-filenames (filename default-filename)
  "Given a filename, determines if the file is generated. Returns a list of
filenames that might be the source file plus the original filename and
default-filename"
  (append (gtags-generate-potential-filenames-helper filename
						      gtags-filename-generators
						      nil)
	  (list filename default-filename)))


;;;;****************************************************************************
;;;; Interactive

(defun gtags-prompt-tag (string &optional language)
  "Prompt the user for a tag using STRING as a prompt.  Returns
the selected tag as a string, or defaults to tag at point if
nothing is typed."
  (let ((original-buffer (current-buffer)))
  ;; gnu-emacs-21's completing-read demands a symbol for the
  ;; completion function, not a lambda, so we bind it here with
  ;; flet.  lexical-let doesn't seem to work here.
  (flet ((completion-function (string predicate operation-type)
            (gtags-completion-collection-function original-buffer
                                                   string
                                                   predicate
                                                   operation-type
                                                   language)))
    (let* ((default (gtags-tag-under-cursor))
           (spec (completing-read
                  (if default (format "%s(default %s) " string default)
                    string)
                  #'completion-function
                  nil
                  nil
                  nil
                  'gtags-find-tag-history
                  default)))
      (if (equal spec "")
          (error "No tag supplied.")
        spec)))))

(defun gtags-language (prompt)
  "Return the default language based on the current buffer and,
if that doesn't yield an answer, then on `gtags-default-mode'.
If PROMPT is supplied, prompt the user instead."
  (let ((default-language (gtags-default-language)))
    (if prompt
        (gtags-prompt-language default-language)
      default-language)))

(defun gtags-default-language ()
  "Return the default language based on the current buffer and, if
that doesn't yield an answer, then on `gtags-default-mode'."
  (car (or (gtags-mode-to-language
            (car (gtags-get-buffer-context (current-buffer))))
           (gtags-mode-to-language gtags-default-mode))))

(defun gtags-prompt-language (default-language)
  "Prompts the user for a language and returns it as a
string.  Defaults to the language of the current buffer (guessed
from the major mode)."
  (let ((spec
         (completing-read (if (not (equal default-language ""))
                              (format "Language (default %s): "
                                      default-language)
                            "Language: ")
                          gtags-language-options
                          nil
                          t)))
    (or (and (not (equal spec ""))
             spec)
        default-language
        (error "No language selected."))))

(defun gtags-tag-under-cursor ()
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

(defun gtags-do-something-with-tag-under-point (if-gtags-property
                                                 if-normal-text)
  (let ((tagrecord (get-text-property (point) 'gtag)))
    (if tagrecord
        (funcall if-gtags-property tagrecord)
      (funcall if-normal-text (gtags-tag-under-cursor)))))

(defun gtags-do-something-with-tag-name-under-point (what-to-do)
  (gtags-do-something-with-tag-under-point
   (lambda (tagrecord)
     (funcall what-to-do
              (gtags-tagrecord-tag tagrecord)))
   what-to-do))

(defcustom gtags-show-tag-location-in-separate-frame nil
  "If turned on, \\[gtags-show-tag-locations-under-point] will pop up a new
frame and show the selected tag. If the file of the selected tag is already
visible in some frame, then re-use that frame and try to raise it."
  :group 'gtags)

(defun gtags-show-tag-locations-under-point ()
  "Show all definitions of the tag around point.
If the text has a `gtag' property, jump to that specific location
rather than looking the tag up. May create a new frame if the
variable `gtags-show-tag-location-in-separate-frame' is set."
  (interactive)
  (gtags-do-something-with-tag-under-point
   '(lambda (tag) (gtags-visit-tagrecord tag t nil nil 'show-tag-location))
   'gtags-show-tag-locations))

(defun gtags-show-callers-under-point ()
  "Show all callers of the tag around point."
  (interactive)
  (gtags-do-something-with-tag-name-under-point 'gtags-show-callers))

(defalias
  'gtags-jump-to-tag-under-point
  'gtags-show-tag-locations-under-point)

(defalias 'gtags-find-tag 'gtags-show-tag-locations)

(defun gtags-visit-javadoc-under-point ()
  "Jump to the Javadoc for the tag around point."
  (interactive)
  (gtags-do-something-with-tag-under-point 'gtags-visit-javadoc
                                            'gtags-javadoc))

(defun gtags-show-tag-locations (tagname &optional language)
  "Show all definitions of TAGNAME.
If there is only one match, jump directly to it. Optionally
specify LANGUAGE to check (if omitted, the major mode and file
extension are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (gtags-language current-prefix-arg)))
                 (list (gtags-prompt-tag "Tag: " language)
                       language)))
  (gtags-get-tags-and-show-results
   'lookup-tag-exact
   `((tag ,tagname)
     (language ,(gtags-guess-language-if-nil language))
     (callers nil)
     (current-file ,buffer-file-name)
     (ranking-methods ,@gtags-ranking-methods))))

(defun gtags-search-definition-snippets (string &optional language)
  "Show all definition snippets which contain the regexp STRING.
Optionally specify LANGUAGE to check (if omitted, the major mode
and file extension are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (gtags-language current-prefix-arg)))
                 (list (gtags-prompt-tag "Search snippets for: " language)
                       language)))
  (gtags-get-tags-and-show-results
   'lookup-tag-snippet-regexp
   `((tag ,string)
     (language ,(gtags-guess-language-if-nil language))
     (callers nil)
     (current-file ,buffer-file-name)
     (ranking-methods ,@gtags-ranking-methods))
   (gtags-make-gtags-buffer-name string
                                  "SNIPPETS")))

(defun gtags-search-caller-snippets (string &optional language)
  "Show all caller snippets which contain the regexp STRING.
Optionally specify LANGUAGE to check (if omitted, the major mode
and file extension are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (gtags-language current-prefix-arg)))
                 (list (gtags-prompt-tag "Search snippets for: " language)
                       language)))
  (gtags-get-tags-and-show-results
   'lookup-tag-snippet-regexp
   `((tag ,string)
     (language ,(gtags-guess-language-if-nil language))
     (callers t)
     (current-file ,buffer-file-name)
     (ranking-methods ,@gtags-ranking-methods))
   (gtags-make-gtags-buffer-name string
                                  "SNIPPETS")
   t))

(defun gtags-show-callers (tagname &optional language)
  "Show all callers of TAGNAME.
Optionally specify LANGUAGE to check (if omitted, the major mode
and file extension are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (gtags-language current-prefix-arg)))
                 (list (gtags-prompt-tag "Tag: " language)
                       language)))
  (gtags-get-tags-and-show-results
   'lookup-tag-exact
   `((tag ,tagname)
     (language ,(gtags-guess-language-if-nil language))
     (callers t)
     (current-file ,buffer-file-name)
     (ranking-methods ,@gtags-ranking-methods))
   (gtags-make-gtags-buffer-name tagname
                                  "CALLERS")
   t))

(defun gtags-show-tag-locations-regexp (tagname &optional language)
  "Show all tags beginning with the regexp TAGNAME.
Optionally specify LANGUAGE to check (if omitted, the major mode
and file extension are used to guess the language).

Interactively, use \\[universal-argument] prefix to be prompted
for language."
  (interactive (let ((language (gtags-language current-prefix-arg)))
                 (list (gtags-prompt-tag "Regexp: " language)
                       language)))
  (gtags-get-tags-and-show-results
   'lookup-tag-prefix-regexp
   `((tag ,tagname)
     (language ,(gtags-guess-language-if-nil language))
     (callers nil)
     (current-file ,buffer-file-name)
     (ranking-methods ,@gtags-ranking-methods))))

(defalias 'gtags-show-matching-tags 'gtags-show-tag-locations-regexp)

(defun gtags-list-tags (filename)
  "List all the tags in FILENAME, defaulting to visited file."
  (interactive "fFilename: ")
  (let* ((filename-only (file-name-nondirectory filename))
         (matches (gtags-get-tags 'lookup-tags-in-file
                                   `((file ,(expand-file-name filename))
                                     (language ,(gtags-guess-language)))
                                   :ranked-p t
                                   :error-if-no-results-p t))
         (uniques (sort* (google-remove-association-dups matches)
                         #'string-lessp
                         :key #'gtags-tagrecord-tag)))
    (gtags-show-results uniques
                              (gtags-make-gtags-buffer-name filename-only))))

(defun gtags-javadoc (tagname)
  "Jump to the first match for TAGNAME in Google's Javadocs."
  (interactive (list (gtags-prompt-tag "Javadoc: " 'java)))
  (gtags-visit-javadoc
   (car
    (gtags-get-tags 'lookup-tag-exact
                     `((tag ,tagname)
                       (callers nil)
                       (current-file ,buffer-file-name))
                     :ranked-p t
                     :error-if-no-results-p t))))

;; TODO(arthur): Provide the user with a way to cull matches before
;; starting the replacement process, e.g. by selecting files from a
;; list of matching files before visiting any matches.
(defun gtags-definitions-and-callers (pattern language)
  (flet ((lookup (callers-p)
                 (gtags-get-tags
                  'lookup-tag-prefix-regexp
                  `((tag ,pattern)
                    (language ,(gtags-guess-language-if-nil language))
                    (callers ,callers-p)
                    (current-file ,buffer-file-name)
                    (ranking-methods ,@gtags-ranking-methods))
                  :caller-p nil
                  :ranked-p t
                  :error-if-no-results-p nil)))
    (append (lookup nil)
            (lookup t))))

(defun gtags-toggle-read-only ()
  (toggle-read-only 1))

(defun gtags-query-replace-read-args (prompt
                                            regexp-flag
                                            &optional noerror)
  (let* ((language (gtags-language current-prefix-arg))
         (from (gtags-prompt-tag prompt language))
         (to (gtags-prompt-tag (format "%sfor %s " prompt from)
                                language)))
    (list from to (gtags-language current-prefix-arg))))

(defun gtags-query-replace (from to &optional language)
  "Do `query-replace-regexp' of FROM with TO on files matching FROM in Gtags.
If a prefix argument is supplied, prompt for the language on which to
match.  Otherwise, guess the language based on the current buffer
or, if that doesn't help, on `gtags-default-mode'.  Skip
matches that are not in the filesystem, e.g. that have been
deleted since the index was created or that were not checked out, but
display them before starting.

If you exit (\\[keyboard-quit], RET or q), you can resume the
query replace with the command \\[tags-loop-continue]."
  (interactive
   (gtags-query-replace-read-args "Gtags query replace: "
                                        t
                                        t))
  (let* ((root (gtags-find-source-root (current-buffer)))
         (all-files
          (remove-duplicates
           (loop for match in (gtags-definitions-and-callers from
                                                                   language)



                 collect (let ((file (substitute-in-file-name
                                      (concat
                                       root
                                       (cadr (assq 'filename match))))))
                           (cons file (file-exists-p file))))
           :test #'equal))
         (existing-files (loop for file in all-files
                               when (cdr file)
                               collect (car file)))
         (existing-files-form `(list ,@existing-files))
         (missing-files (loop for file in all-files
                              when (not (cdr file))
                              collect (car file))))
    (if missing-files
        (with-output-to-temp-buffer "*gtags-query-replace*"
          (princ "The following files matched in the Gtags index but were\n")
          (princ "not present among your files.  They will be skipped.\n\n")
          (loop for file in missing-files
                do (princ (format "%s\n" file)))))
    (setq tags-loop-scan `(let ,(unless (equal from (downcase from))
                                  '((case-fold-search nil)))
                            (if (re-search-forward ',from nil t)
                                (goto-char (match-beginning 0)))))
    (setq tags-loop-operate
          `(flet ((replace-match (newtext
                                  &optional fixedcase literal string subexp)
                    (if buffer-read-only (gtags-toggle-read-only))
                    (funcall ,(symbol-function 'replace-match)
                             newtext
                             fixedcase
                             literal
                             string
                             subexp)))
             (perform-replace ',from ',to t t nil)))
    (tags-loop-continue (or existing-files-form t))))

(defun gtags-visit-tagrecord (tagrecord
                               &optional
                               push
                               display-buffer
                               buffer
			       show-tag-location)
  "Given a tag record, jump to the line/file.
If PUSH is non-nil then push the current location on a stack.  If
DISPLAY-BUFFER is non-nil then display the result in
gdoc-display-window.  If BUFFER is non-nil then we replace the
contents of that buffer with the file contents (which is good for
processing lots of tags in succession).  If SHOW-TAG-LOCATION is
non-nil then possibly pop up a new frame.  See the variable
`gtags-show-tag-location-in-separate-frame'.  This function is
called from various functions in this file, as well as
gdoc-mode.el The optional arguments are used to control desired
functionality as seen from each individual caller."
  (let* ((tagname (gtags-tagrecord-tag tagrecord))
         (filename (gtags-tagrecord-filename tagrecord))
         (snippet (gtags-tagrecord-snippet tagrecord))
         (lineno (gtags-tagrecord-lineno tagrecord))
         (offset (gtags-tagrecord-offset tagrecord))
         (etagrecord (cons snippet (cons lineno offset)))
         (actual-filename
          (expand-file-name
           (concat (gtags-directory-truename
                    (gtags-find-source-root default-directory))
                   filename)))
         (default-filename
           (expand-file-name
            (concat (gtags-directory-truename gtags-default-root)
                    filename)))
         (selected-file (gtags-select-from-files
                         (cons filename (gtags-generate-potential-filenames
                                         actual-filename
                                         default-filename))))
	 ;; If selected-file is none of filename, actual-filename or
         ;; default-filename then it must be a generated file.
         (generated-file (not (or
                               (equal filename selected-file)
			       (equal actual-filename selected-file)
			       (equal default-filename selected-file)))))
    (if push (gtags-push-tag))  ; Save the tag so we can come back.
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
		       gtags-show-tag-location-in-separate-frame)
		  (gtags-find-file-other-frame selected-file)
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
	     gtags-show-tag-location-in-separate-frame)
      (gtags-raise-frame-of-file selected-file))))


;;;;****************************************************************************
;;;; Mixer

(defvar gtags-use-gtags-mixer t
  "Whether to use gtags mixer.")

(defvar gtags-mixer-port 2220)

(defvar gtags-mixer-command
  "/path/to/gtagsmixer")

(defun gtags-start-gtags-mixer ()
  "Start the GTags mixer daemon."
  (interactive)
  (call-process gtags-mixer-command nil nil nil
                "--port" (number-to-string gtags-mixer-port)
                "--fileindex")
  (setq gtags-use-gtags-mixer t)
  (memoize-forget 'gtags-find-host-port-pair))

(defun gtags-restart-gtags-mixer ()
  "Restart the GTags mixer daemon, replacing the old mixer."
  (interactive)
  (call-process gtags-mixer-command nil nil nil
                "--port" (number-to-string gtags-mixer-port)
                "--fileindex"
		"--replace")
  (setq gtags-use-gtags-mixer t)
  (memoize-forget 'gtags-find-host-port-pair))

(defun gtags-stop-gtags-mixer ()
  "Stop the GTags mixer daemon."
  (interactive)
  (setq gtags-use-gtags-mixer nil)
  (memoize-forget 'gtags-find-host-port-pair))

(defadvice gtags-cannot-connect (around handle-mixer-error activate)
  "Report a mixer-specific error if we're using the mixer."
  (if gtags-use-gtags-mixer
      (error "Cannot connect to mixer.  %s"
             "Please type M-x gtags-restart-gtags-mixer and try again.")
    ad-do-it))

;; Start mixer on load unless user set gtags-use-gtags-mixer to nil.
;; Also wait 100ms until gtagsmixer finishes initializing.
(when gtags-use-gtags-mixer
  (gtags-start-gtags-mixer)
  (sleep-for 0.1))


;;;;****************************************************************************
;;;; Utility Functions

(defun gtags-select-from-files (file-list)
  "Return the first file in FILE-LIST that exists."
  (find-if '(lambda (x) (file-exists-p x)) file-list))

(defun gtags-rename-buffer (old-name new-name)
  (save-excursion
    (set-buffer old-name)
    (rename-buffer new-name)))

(defun gtags-longest-common-prefix (string1 string2)
  (do ((index 0 (+ index 1)))
      ((or (= index (length string1))
           (= index (length string2))
           (not (= (aref string1 index)
                   (aref string2 index))))
       (substring string1 0 index))))

(defun gtags-trim-leading-whitespace (string)
  "Return STRING without leading whitespace."
  (if (string= string "")
      string
    (save-match-data
      (if (string-match "^\\s-+" string)
          (substring string (match-end 0))
        string))))

(defun gtags-trim-trailing-whitespace (string)
  "Return STRING without trailing whitespace."
  (if (string= string "")
      string
    (save-match-data
      (if (string-match "\\s-+$" string)
          (substring string 0 (match-beginning 0))
        string))))

(defun gtags-trim-whitespace (string)
  "Return STRING without leading or trailing whitespace."
  (gtags-trim-trailing-whitespace
   (gtags-trim-leading-whitespace string)))

(defun gtags-make-gtags-buffer-name (about &optional prefix)
  (format "*%s: %s*"
          (or prefix "TAGS")
          about))

(defun gtags-directory-truename (filename)
  "Call file-truename on filename, then make sure it ends with a /."
  (file-name-as-directory (file-truename filename)))

(defun gtags-raise-frame-of-file (filename)
  "Raise the frame displaying FILENAME."
  (interactive "fFilename :")
  (if (gtags-xemacs)
      (gtags-xemacs-raise-frame-of-file filename)
    (gtags-gnuemacs-raise-frame-of-file filename)))

(defun gtags-find-file-other-frame (filename)
  "Edit FILENAME in another frame, reuse existing frame if possible."
  (interactive "fFilename :")
  (if (gtags-xemacs)
      (gtags-xemacs-find-file-other-frame filename)
    (gtags-gnuemacs-find-file-other-frame filename)))


;;;;****************************************************************************
;;;; GNU Emacs-specific utility functions

(defun gtags-gnuemacs-find-file-other-frame (filename)
  "Edit FILENAME in another frame, reuse existing frame if possible."
  (interactive "fFilename :")
  (let ((display-buffer-reuse-frames t))
    (find-file-other-frame filename)))

;; In gnu emacs raise-frame only works if the desired frame has input focus.
(defun gtags-gnuemacs-raise-frame-of-file (filename)
  "Raise the frame displaying FILENAME."
  (interactive "fFilename :")
  (let* ((buffer  (get-file-buffer filename))
	 (window  (get-buffer-window buffer t))
	 (frame   (window-frame window)))
    (gtags-gnuemacs-save-frame-excursion
      (select-frame-set-input-focus frame)
      (raise-frame frame))))

;; This is basically the same as save-window-excursion,
;; except that we save current-frame information, rather than window info.
(defmacro gtags-gnuemacs-save-frame-excursion (&rest body)
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

(defun gtags-gnuemacs-show-all-overlays ()
  (dolist (overlay (overlays-in (point-min) (point-max)))
    (overlay-put overlay 'invisible nil)))


;;;;****************************************************************************
;;;; Xemacs-specific utility functions

(defun gtags-xemacs-find-file-other-frame (filename)
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

;; In xemacs raise-frame works if we have done select-window first.
(defun gtags-xemacs-raise-frame-of-file (filename)
  "Raise the frame displaying FILENAME."
  (interactive "fFilename :")
  (let* ((buffer  (get-file-buffer filename))
	 (window  (get-buffer-window buffer t))
	 (frame   (window-frame window)))
    (save-window-excursion
      (select-window window)
      (raise-frame frame))))

(defun gtags-xemacs-show-all-overlays ()
  (dolist (extent (extent-list))
    (set-extent-property extent 'invisible nil)))

(defun gtags-xemacs-mark-extent (start end &optional face)
  (let ((xemacs-region (make-extent start end)))
    (if face
        (set-extent-property xemacs-region 'face face))
    (set-extent-property xemacs-region
                         'mouse-face
                         'widget-button-face)))


;;;;****************************************************************************
;;;; Browse Tags mode

(define-derived-mode gtags-browse-tags-mode fundamental-mode
  "GTAGS"
  "Major mode for browsing Google tags.
\\[gtags-visit-javadoc-under-point] to jump Javadoc for a particular tag
\\[gtags-show-tag-locations-under-point] to jump to a particular tag
\\[gtags-narrow-by-filename] to perform a narrowing
\\[gtags-show-all-results] to show all tags
\\[delete-window] to quit browsing")

;;; Commands

(defalias 'gtags-visit-tag-under-point 'gtags-show-tag-locations-under-point)

(defun gtags-xemacs-visit-tag-pointed-by-mouse ()
 "Weird hack to let Xemacs work properly with the mouse."
  (interactive)
  (mouse-set-point current-mouse-event)
  (gtags-show-tag-locations-under-point))

(defun gtags-narrow-by-filename (regexp)
  "In a GTAGS buffer, narrow the view to files matching REGEXP."
  (interactive "sNarrow to filenames matching: ")
  (if (gtags-xemacs) ; Xemacs and GNU emacs are completely different.
      (gtags-xemacs-narrow-by-filename regexp)
    (gtags-gnuemacs-narrow-by-filename regexp)))

(defun gtags-gnuemacs-narrow-by-filename (regexp)
  (dolist (overlay (overlays-in (point-min) (point-max)))
    (let* ((start-prop (overlay-start overlay))
           (tagrecord (get-text-property start-prop 'gtag))
           (filename (gtags-tagrecord-filename tagrecord)))
      (if (not (string-match regexp filename))
          (overlay-put overlay 'invisible t)))))

(defun gtags-xemacs-narrow-by-filename (regexp)
  (dolist (extent (extent-list))
    (let* ((startpos (extent-start-position extent))
           (endpos (extent-end-position extent))
           (tagrecord (get-text-property startpos 'gtag)))
      (if tagrecord
          (let ((filename (gtags-tagrecord-filename tagrecord)))
            (if (not (string-match regexp filename))
                (set-extent-property extent 'invisible t)))))))

(defun gtags-show-all-results ()
  "After `gtags-narrow-by-filename', restores all matches."
  (interactive)
  (if (gtags-xemacs)
      (gtags-xemacs-show-all-overlays)
    (gtags-gnuemacs-show-all-overlays)))

;;; Key and mouse bindings

(define-key gtags-browse-tags-mode-map
  "\C-j"
  'gtags-visit-javadoc-under-point)

(define-key gtags-browse-tags-mode-map
  "\C-m"
  'gtags-show-tag-locations-under-point)

(define-key gtags-browse-tags-mode-map
  "a"
  'gtags-show-all-results)

(define-key gtags-browse-tags-mode-map
  "n"
  'gtags-narrow-by-filename)

(define-key gtags-browse-tags-mode-map
  "q"
  'delete-window)

(define-key gtags-browse-tags-mode-map
  "h"
  'describe-mode)

(if (gtags-xemacs)
    (define-key gtags-browse-tags-mode-map
      [button1]
      'gtags-xemacs-visit-tag-pointed-by-mouse)
  (define-key gtags-browse-tags-mode-map
    [mouse-1]
    'gtags-show-tag-locations-under-point))

(provide 'gtags)

(require 'gtags-aliases)
