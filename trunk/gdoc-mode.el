;;; Gdoc mode.  This was Piaw's idea and is Arthur's implementation,
;;; Authors: Arthur A. Gleckler, Piaw Na
;;; Copyright Google Inc. 2005
;;; inspired by Eldoc mode.

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


;;; $Id: //depot/opensource/gtags/gdoc-mode.el#4 $

(require 'gtags)

(defconst gtags-gdoc-server-buffer-name " *TAGS server (Gdoc) *"
  "Buffer used for communication with gtags server.  Separate from
`gtags-server-buffer-name' to prevent conflicts.")

(defcustom gdoc-minor-mode-string " GDoc"
  "*String to display in mode line when Gdoc Mode is enabled."
  :type 'string
  :group 'gdoc)

(defcustom gdoc-mode-list '(c++-mode java-mode jde-mode python-mode)
  "*Modes of buffers in which Gdoc mode should be active."
  :type '(restricted-sexp :match-alternatives (symbolp))
  :group 'gdoc)

(defcustom gdoc-idle-delay 0.50
  "*Number of seconds of idle time to wait before printing.
If user input arrives before this interval of time has elapsed after the
last input, nothing will be printed.

If this variable is set to 0, no idle time is required."
  :type 'number
  :group 'gdoc)

(defcustom gdoc-pop-up-frames t
  "*whether to use pop-up frames for showing results in Gdoc mode"
  :type 'boolean
  :group 'gdoc)

(defcustom gdoc-display-definitions t
  "*whether or not to display definitions*"
  :type 'boolean
  :group 'gdoc)

(defvar gdoc-display-window nil
  "*display window for the gdoc mode*")

(add-minor-mode 'gdoc-mode 'gdoc-minor-mode-string)

(defvar gdoc-timer nil)

(define-minor-mode gdoc-mode
  "Toggle Gdoc, a global minor mode.  Interactively, with no prefix
argument, toggle the mode.  With universal prefix ARG turn mode on.
With zero or negative ARG turn mode off.When gdoc mode is active,
Emacs continuously looks up the tag under the point."
  nil
  gdoc-minor-mode-string
  nil)

(defun gdoc-set-window ()
  (if (not (and (windowp gdoc-display-window)
		(window-live-p gdoc-display-window)))
      (setq gdoc-display-window (if gdoc-pop-up-frames
				    (frame-first-window (make-frame))
				  (display-buffer "*TAGS*" t t)))))

(add-hook 'gdoc-mode-hook 'gdoc-set-window)
(add-hook 'gdoc-mode-hook 'gdoc-schedule-timer)

(defun turn-on-gdoc-mode ()
  "Unequivocally turn on gdoc-mode (see variable documentation)."
  (interactive)
  (gdoc-mode 1))

(defun gdoc-schedule-timer ()
  (or (and gdoc-timer
           (memq gdoc-timer timer-idle-list))
      (setq gdoc-timer
            (run-with-idle-timer gdoc-idle-delay
                                 t
                                 'gdoc-print-current-tag-info))))

(defun gdoc-cancel-timer ()
  (when gdoc-timer
    (cancel-timer gdoc-timer)
    (setq gdoc-timer nil)))

(defun class-or-struct-p (matches tag)
  "Returns the first tag-record if any of the matches represents
a struct or a class. Note that this is just a heuristic. It will
not be correct in all cases."
  (find-if
   '(lambda (x) (let* ((tagname (tagrecord-tag x))
		       (data (tagrecord-data x))
		       (snippet (tagrecord-data-snippet data)))
		  (and (equal tagname tag)
		       (string-match (concat "\\(struct\\|class\\) +" tag)
				     snippet))))
   matches))

;; TODO(arthur): Make this asynchronous so that it will never block,
;; even if the server is slow or not responding.  So far, this hasn't
;; been a problem, but we should anticipate the problem.

(defun google-show-tag-locations-continuous (tagname
                                             regexp-pred
                                             opcode
                                             buffer-name)
  "Show all matches for `tagname' in a buffer named `buffer-name' in
another window (or in another frame, if `pop-up-frames' is non-nil).

`Regexp-pred' is a function that takes a tagrecord and returns non-nil
if it matches.

`Opcode' specifies which server \"opcode\" operation to perform,
e.g. `:' for direct tag lookup."
  (let* ((gtags-server-buffer-name gtags-gdoc-server-buffer-name)
	 (current-directory default-directory)
	 (matches
	  (google-get-unique-matches opcode tagname regexp-pred)))
    (if (and gdoc-display-definitions
	     (= (length matches) 1))  ; unique match!
	   (google-visit-tagrecord (car matches) nil buffer-name)
      (let ((class-record (class-or-struct-p matches tagname)))
	(when (and gdoc-display-definitions
		   class-record)      ;; there's a match for class or struct
	  (google-visit-tagrecord class-record nil buffer-name))
	(when (not (zerop (length matches)))
	  (google-show-tag-locations-inner
	   matches
	   tagname
	   buffer-name
	   current-directory
	   gdoc-display-window))))))

;; TODO(arthur) Fix: When the pop-up window is first created, it gets
;; focus.  I don't know how to fix this, as it seems to be controlled
;; by the window manager, not Emacs.  Even using `select-frame'
;; afterwards doesn't help.

(defun gdoc-print-current-tag-info ()
  "Show matches for tag under point."
  (let ((current-frame (selected-frame)))
    (setq global-frame current-frame)
    (if (and gdoc-mode
	     (memq major-mode gdoc-mode-list))
	(let ((pop-up-frames gdoc-pop-up-frames))
	  (google-show-tag-locations-continuous
	   (google-find-default-tag)
	   '(lambda (x) (string-equal tagname (tagrecord-tag x)))
	   gtags-opcode-exact
	   "*TAGS*")))
    (if (google-xemacs)
	;; HACK! xemacs does not do save-selected-frame well at all,
	;; so we have to resort to refocusing to the current-frame
	;; for some wierd reason this only shows up in Java mode
	;; save-selected-frame works fine in C++ mode, but
	;; this hack does not affect c++-mode badly
	(focus-frame current-frame))))


(provide 'gdoc-mode)
