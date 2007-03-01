;;;; Copyright 2005-2006 Google Inc.

;;;; Emacs Utilities for Google

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


;;; Cache results of function calls.  Note that this does not notice
;;; when the function or one of the functions it calls changes.
;;;
;;; To use this on the function `foo':
;;;
;;;   (memoize 'foo)
;;;
;;; This will memoize just the latest result, and will keep it
;;; forever.  To remember the last ten results for the next half hour:
;;;
;;;   (memoize 'foo 10 (* 30 60))
;;;
;;; To turn the cache off, do: (unmemoize 'foo)

(require 'cl)

(defstruct (memo)
  arguments
  result
  expiration)

;; TODO(arthur): Add an optional parameter that allows the caller to
;; choose the argument-comparison.  This version always uses `equal'.
(defmacro memoize (function-name &optional max-results interval-seconds)
  "Cache results of calls to `function-name'.  Note that this does not
notice when the function or one of the functions it calls changes."
  (let ((function-name (eval function-name))
        (interval-seconds (eval interval-seconds))
        (max-results (or (eval max-results) 1)))
    `(progn
       (memoize-forget ',function-name)
       (defadvice ,function-name (around memoize activate)
         ,(format
           "Memoize the results of the most recent %s%s
when all arguments match.  Does not notice when the function itself or
one of the functions it calls changes."
           (if (= max-results 1)
               "call"
             (format "%s calls" max-results))
           (if interval-seconds
               (format " for %s seconds" interval-seconds)
             ""))
         (let* ((arguments (ad-get-args 0))
                (existing-results
                 (get ',function-name 'memoized-results))
                (memo
                 (find arguments
                       existing-results
                       :key #'memo-arguments
                       :test #'equal))
                (now (let ((time (current-time)))
                       (+ (* (car time) 65536)
                          (cadr time)))))
           (if memo
               (setq ad-return-value
                     (memo-result memo))
             (let ((new-result ad-do-it))
               (put ',function-name
                    'memoized-results
                    (update-memoized-results existing-results
                                             arguments
                                             new-result
                                             ,max-results
                                             ,interval-seconds
                                             now)))))))))

(defun update-memoized-results (existing-results
                                arguments
                                new-result
                                max-results
                                interval-seconds
                                now)
  (let ((expiration (and interval-seconds (+ now interval-seconds))))
    (cons (make-memo :arguments arguments
                     :result new-result
                     :expiration expiration)
          (let ((accumulator '())
                (count 1)
                (remaining existing-results))
            (while (and (not (null remaining))
                        (< count max-results))
              (let ((first-result (car remaining)))
                (when (not (or (and expiration
                                    (> now (memo-expiration first-result)))
                               (equal arguments (memo-arguments first-result))))
                  (push first-result accumulator)
                  (incf count))
                (setq remaining (cdr remaining))))
            (nreverse accumulator)))))

(defun memoize-forget (function-name)
  "Forget the memoized results for FUNCTION-NAME, but don't stop
memoizing."
  (put function-name 'memoized-results nil))

(defun unmemoize (function-name)
  (interactive "aFunction: ")
  (ad-unadvise function-name)
  (memoize-forget function-name))

;;; Filters, etc.

(defun google-filter (pred list)
  "Return a list consisting of only those elements x of LIST for which
 (PRED x) is non-nil.  Use 'identity' predicate to remove nil elements."
  (let ((result ()))
    (dolist (item list)
      (if (funcall pred item)
          (setq result (cons item result))))
    (nreverse result)))

(defun google-remove-association-dups (alist)
  (let ((result nil))
    (dolist (association alist)
      (if (not (assoc (car association) result))
          (setq result (cons association result))))
    ;;; The above process reversed the results,
    ;;; so now we reverse it back.
    (reverse result)))

;;; Processes

(defun google-find-buffer-with-prefix (prefix)
  "Find some buffer whose name starts with `prefix' and which has an
active process.  This function is usually used under the assumption
that there is only one such buffer, or that the user will notice if
the wrong one is chosen."
  (let ((prefix-length (length prefix)))
    (some
     (lambda (buffer)
       (let ((name (buffer-name buffer)))
         (and (eq t (compare-strings name
                                     0
                                     prefix-length
                                     prefix
                                     0
                                     prefix-length))
              (let ((process
                     (find buffer (process-list) :key #'process-buffer)))
                (and process
                     (eq (process-status process) 'run)
                     buffer)))))
     (buffer-list))))

(provide 'google-emacs-utilities)
