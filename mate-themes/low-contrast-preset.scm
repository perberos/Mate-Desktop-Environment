;;; The GIMP -- an image manipulation program
;;; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;;;
;;; script-fu-low-contrast-preset
;;; Loads a PNG, makes it low contrast, saves it
;;; Copyright (c) 2002 Guillermo S. Romero
;;; famrom infernal-iceberg.com
;;;
;;; ### License  ###
;;;
;;; This program is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;; 
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;; 
;;; You can get a copy of the GNU General Public License from
;;; http://www.gnu.org/, the site of the Free Software Foundation, Inc.
;;;
;;; ### Versions ###
;;;
;;; 1.00.00 Initial release
;;; 1.00.01 Fixed typo 
;;;
;;; ### Inline Documentation ###
;;;
;;; # Intro #
;;; Some people have eye problems and require low contrast images.
;;; This script helps converting normal icons to low contrast, great for
;;; those persons and for developers that want to ship special icon sets.
;;;
;;; # Full usage #
;;; Place low-contrast-preset.scm in the GIMP script dir, for example
;;; ~/.gimp-1.2/scripts/.
;;; Next copy all the icons to somewhere (so you keep the originals)
;;; and there:
;;; for i in *.png
;;; do
;;;     gimp -i -s -b "(script-fu-low-contrast-preset \"${PWD}${i}\")" \
;;;                   "(gimp-quit 0)"
;;; done
;;; It will take a while and must be run from inside a X11 session
;;; (Xnest and Xvfb are ok) if you are using GIMP 1.2.
;;;
;;; # Requirements #
;;; This plugin was developed with Gimp 1.2.3 CVS
;;; Check all your plugins and scripts in the DB Browser before reporting bugs
;;;
;;; # Parameters #
;;; filename: a path to a PNG, like default vale
;;;
;;; # Debugging and to-do #
;;; Look for ";; @" comments

;; The function, hardcoded values for now
(define (script-fu-low-contrast-preset filename)

  (let* ((image (car (file-png-load 1 filename filename)))
         (drawable (car (gimp-image-active-drawable image))))
    (gimp-levels drawable 0 0 255 1.0 125 175)
    (file-png-save 1 image drawable filename filename 0 9 0 0 0 1 1)))

;; Register!
(script-fu-register "script-fu-low-contrast-preset"
		    "<Toolbox>/Xtns/Script-Fu/Misc/Low contrast PNG icon preset..."
		    "Loads a PNG, makes it low contrast, saves it"
		    "Guillermo S. Romero"
		    "2002 Guillermo S. Romero"
		    "2002-09-12"
		    ""
		    SF-FILENAME "PNG to modify" "/tmp/icon.png")
