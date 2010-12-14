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
          (call-interactively 'undertaker::start)
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
                                      args (list "\n")))))))))

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

(defun undertaker::start (cmdline)
  "Starts an undertaker process in the current directory"
  (interactive
   (list
    (read-string "undertaker::start> " "~/vamos/undertaker/undertaker -j blockpc -")))
  (if (undertaker::process-runningp)
      (message "undertaker ist running already, use undertaker::stop to kill it")
    (progn
      (setq undertaker::process
            (eval
             `(start-process "undertaker" "*undertaker*" ,@(split-string cmdline))))
      ;; turn on the undertaker output mode, in order to get a little
      ;; bit of syntax highlighting
      (with-current-buffer "*undertaker*"
        (undertaker-output-mode))
      (undertaker::show-process))))

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
    (undertaker::clear-buffer)
    (undertaker::show-process)
    (message input)
    (let ((dir default-directory))
      (with-current-buffer (process-buffer undertaker::process)
        (process-send-string undertaker::process input)
        (accept-process-output undertaker::process 0 500)
        (beginning-of-buffer)
        (insert (concat "Current Directory: "
                        dir "\nInput: " input "\n"))))))

(defun undertaker::clear-buffer ()
  "Clears the undertaker buffer, if it exists"
  (when (undertaker::process-runningp)
    (with-current-buffer (process-buffer undertaker::process)
      (delete-region (point-min) (point-max)))))


(define-generic-mode
  'undertaker-output-mode                         ;; name of the mode to create
  '("I:")                           ;; comments start with '!!'
  nil                               ;; keywords
  '(("&&" . 'font-lock-operator)     ;; '&&' is an operator
    ("||" . 'font-lock-operator)
    ("<->" . 'font-lock-operator)
    ("->" . 'font-lock-operator)
    ("!" . 'font-lock-operator)
    ("\\<B[0-9]+\\>" . 'font-lock-type-face)
    ("\\<CONFIG_[0-9A-Za-z_]*\\>" . 'font-lock-keyword-face))
  ;; ';' is a built-in
  nil                     ;; files for which to activate this mode
  nil                              ;; other functions to call
  "A mode for the output of undertaker"            ;; doc string for this mode
  )


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
  '(("ub" . undertaker::current-blockpc)
    ("us" . undertaker::current-symbolpc))
  :group 'undertaker
)