# YAAWS
Yet Another Arduino Web Server

As part of my first Arduino project, I wanted to be able to monitor and control it from a web browser.  I could have used or adapted an existing solution, but where's the fun in that?

Thus was born YAAWS, Yet Another Arduino Web Server.  I'm releasing it hoping that others might find it useful.

YAAWS provides:
  - GET / HEAD support
  - POST support
  - Reduced blocking
  - Website can be any size, limited only by what you can fit on the SD card
  - Proper 'Content-type' support for most common files
  - Multiple simultaneous connections
  - Limited Dynamic HTML support.

  To reduce blocking of the rest of your sketch, file transfers are done in reasonably small chunks.  No matter how big the file to be processed, YAAWS will not block for any great length of time.

  In addition, you can disable or reduce some functionality to save RAM or FLASH space.

  Examples show basic usage for simple web sites and form processing for both GET and POST style forms, and simplistic dynamic HTML.  More examples coming!

  YAAWS is currently sitting at version 1.0.0.  Available from the Arduino IDE Library Manager, or directly from GitHub at: https://github.com/MHotchin/YAAWS
