(defun get-buffer-mode-str (buffer-name)
  (format "%s" (save-excursion
                 (set-buffer buffer-name)
                 major-mode)))

(defmacro buffer-mode-match (str name)
  `(string-match ,str (get-buffer-mode-str name)))

(defun get-color-name (name)
  (cond
   ((buffer-mode-match "jabber" name) "#cfb53b")
   (t "")))

(defun get-icon-name (name)
  (cond
   ((buffer-mode-match "jabber" name) "im-jabber")
   ((buffer-mode-match "haskell" name) "text-x-haskell")
   ((buffer-mode-match "python" name) "text-x-python")
   ((buffer-mode-match "sh" name) "application-x-shellscript")
   ((buffer-mode-match "c-mode" name) "text-x-csrc")
   (t "emacs")))
