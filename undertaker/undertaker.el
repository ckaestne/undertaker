(require 'thingatpt)
(require 'generic-x) ;; generic mode

(defvar undertaker::process nil)

(defun undertaker::process-runningp ()
  "true if undertaker process is running"
  (and undertaker::process
       (eq (process-status undertaker::process) 'run)))

(defun undertaker::ensure-running ()
  "Checks if undertaker is running and start it otherwise (ask the user)"
  (if (not (undertaker::process-runningp))
      (if (y-or-n-p "No undertaker running. Start it? ")
          (call-interactively 'undertaker::shell)
        (progn
          (message "No undertaker process running -- undertaker::start")
          (return)))
    t)) ;; undertaker is running already

(defun undertaker::send-command (command &rest args)
  "Sends a command with given args to the undertaker process.\n
 Ask for starting if it isn't running"
  (when (undertaker::ensure-running)
   (if args
       (save-excursion
         (undertaker::send-string
          (eval (cons 'concat (append (if command (list command " "))
                                      args))))))))

(defun undertaker::current-blockpc ()
  "Send current position in file to undertaker"
  (interactive)
  (let ((position (undertaker::get-position-at-point)))
    (if position
        (undertaker::send-command "::blockpc" position))))

(defun undertaker::current-symbolpc ()
  "Send current symbol at point to the undertaker"
  (interactive)
  (let ((symbol (thing-at-point 'symbol)))
    (if symbol
        (undertaker::send-command "::symbolpc" symbol))))

(defun undertaker::current-interesting-symbols ()
  "Send current symbol at point to the undertaker (with ::interesting)"
  (interactive)
  (let ((symbol (thing-at-point 'symbol)))
    (if symbol
        (undertaker::send-command "::interesting" symbol))))


(defun undertaker::show-process ()
  "Show the process buffer, if process is running"
  (if (undertaker::process-runningp)
      (display-buffer (process-buffer undertaker::process))))

(defun undertaker::process-cwd ()
  "Get the working directory of the undertaker process"
  (with-current-buffer
      (process-buffer undertaker::process)
    default-directory))

(defun undertaker::stop ()
  "kills the undertaker process"
  (interactive)
  (if (undertaker::process-runningp)
      (progn
        (kill-buffer (process-buffer undertaker::process))
        (message "undertaker killed."))
    (message "No undertaker running")))


(defun undertaker::get-position-at-point ()
  "In the current buffer, get filename:linum as string"
  (when (undertaker::ensure-running)
   (let ((filename (buffer-file-name (current-buffer)))
         (undertaker-path (file-truename (undertaker::process-cwd))))
     (if (string-equal (subseq filename 0 (length undertaker-path))
                       undertaker-path)
         (setq filename (subseq filename (length undertaker-path))))
     (when filename
       (concat filename ":"
               (int-to-string (line-number-at-pos (point))))))))

(defun undertaker::send-string (input)
  "Send exactly this string to the undertaker process"
  (when (undertaker::process-runningp)
    (undertaker::shell nil)
    (let ((dir default-directory))
      (with-current-buffer (process-buffer undertaker::process)
        (save-excursion
          (end-of-buffer)
          (insert input)
          (comint-send-input))))))

(defvar undertaker::toggle-buffer-old nil)
(defun undertaker::toggle-buffer ()
    "Toggles to undertaker output buffer or back to source buffer"
    (interactive)
    (if (and undertaker::toggle-buffer-old
             (not (eq (process-buffer undertaker::process)
                      undertaker::toggle-buffer-old)))
        (progn
          (pop-to-buffer undertaker::toggle-buffer-old)
          (setq undertaker::toggle-buffer-old nil))
      (progn
        (setq undertaker::toggle-buffer-old (current-buffer))
        (pop-to-buffer "*undertaker*"))))

(define-minor-mode undertaker-mode
  "Toggle undertaker mode.
With no argument, this command toggles the mode.
Non-null prefix argument turns on the mode.
Null prefix argument turns off the mode.

When undertaker mode is enabled. Keybindings for sending
the actual position or different informations about your
source files direct to the undertaker process."
  :init-value nil
  :lighter " UT"
  :keymap
  '(([tab]  . comint-dynamic-complete)
    ("ub" . undertaker::current-blockpc)
    ("us" . undertaker::current-symbolpc)
    ("ui" . undertaker::current-interesting-symbols)
    ("uu" . undertaker::toggle-buffer))
  :group 'undertaker
  )

(define-derived-mode undertaker-shell-mode comint-mode "UT-Sh")
(defun undertaker::shell (command)
  (interactive (list nil))
  (unless
      (comint-check-proc "*undertaker*")
    (if (not command)
        (setq command (read-string "undertaker::start> " "~/vamos/undertaker/undertaker -j blockpc -")))
    (setq command (split-string command))
    ;; start process
    (setq comint-prompt-read-only t)
    (setq comint-postoutput-scroll-to-bottom t)
    (setq undertaker::process
          (get-buffer-process
           (eval `(make-comint-in-buffer "undertaker" "*undertaker*" ,(car command) nil ,@(cdr command)))))
    (with-current-buffer "*undertaker*"
      (undertaker-shell-mode)
      (undertaker-mode)
      (setq font-lock-keywords undertaker-font-lock-keywords)))
  (display-buffer "*undertaker*"))

(defvar undertaker-font-lock-keywords
  '(("\\<[A-Z_][A-Z0-9_]+\\>" 0 font-lock-keyword-face t)
    ("\\<B[0-9]+\\>" 0 font-lock-type-face t)
    ("::[a-z]+" 0 font-lock-string-face t)
    ("^I:.*$" 0 font-lock-comment-face t))
  "Additional expressions to highlight in undertaker process buffers.")