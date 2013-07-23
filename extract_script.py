import re
import sys

with open(sys.argv[1], "r") as f:
    buf=f.read()
    while 1:
        m=re.search(r"<script [^>]*>|<script>", buf, re.DOTALL|re.MULTILINE)	
        if m==None:
            break                
        buf=buf[m.end(0):]
        m=re.search(r"</script [^>]*>|</script>", buf, re.DOTALL|re.MULTILINE)
        if m==None:
            break
        print buf[:m.start(0)]
        print
        buf=buf[m.end(0):]