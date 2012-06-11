(require 'cl)

(defvar xwm-modes-alist   
      '(("jabber" "im-jabber" "#cfb53b")
        ("haskell" "text-x-haskell")
        ("python" "text-x-python")
        ("sh" "application-x-shellscript")
        ("c-mode" "text-x-csrc")
        ("" "emacs")))


(defun xwm-buffer-mode-str (buffer-name)
  (format "%s" 
          (with-current-buffer buffer-name major-mode)))

(defun xwm-mode-icon-color (name)
  (cdar (remove-if-not (lambda (ic) 
                         (string-match (car ic) (xwm-buffer-mode-str name)))
                       xwm-modes-alist)))

(defun xwm-valid-buffer-list () 
  (remove-if '(lambda (name) (string= (substring name 0 1) " "))
             (mapcar 'buffer-name (buffer-list))))

(defun xwm-list-buffers ()
  (mapconcat  '(lambda (s) 
                 (let ((ic-list (xwm-mode-icon-color s)))
                   (format ", %s,%s,%s,%s"
                           (or (second ic-list) "")
                           (first ic-list)
                           s
                           (xwm-buffer-mode-str s))))
            (xwm-valid-buffer-list)
            "\n"))

