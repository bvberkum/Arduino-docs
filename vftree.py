#!/usr/bin/env python  
# -*- coding: utf-8 -*-  
"""
- Copy/paste work in progress. Looks like I'll need proper read.py first.

/
    nodes
        <sketch-id>.ini
            <sensor-id>: config
        <sketch-id>/
            <sensor-id>: config
    values
        <sketch-id>
            <sensor-id> -> value

    logs
        <sketch-id>
            <sensor-id>

"""
import errno  
import fuse  
import stat  
import time  

fuse.fuse_python_api = (0, 2)  

class MyFS(fuse.Fuse):  

    def __init__(self, *args, **kw):  
        fuse.Fuse.__init__(self, *args, **kw)  

    def getattr(self, path):
        open("/tmp/mypyfuse", "rw+").write(path)
        st = fuse.Stat()  
        st.st_mode = stat.S_IFDIR | 0755  
        st.st_nlink = 2  
        st.st_atime = int(time.time())  
        st.st_mtime = st.st_atime  
        st.st_ctime = st.st_atime  

        if path == '/':  
            pass  
        else:  
            return - errno.ENOENT  
        return st  

    def readdir(self, path, offset):
        if path == '/':
            yield fuse.Direntry('.')
            yield fuse.Direntry('..')
            for node in self.nodes:
                yield fuse.Direntry(node)

    def open(self, path, flags):
        access_flags = os.O_RDONLY | os.O_WRONLY | os.O_RDWR
        if flags & access_flags != os.O_RDONLY:
            return -errno.EACCES
        else:
            return 0

    def read(self, path, size, offset):
        entry = self.tree.find(path)  
        content = entry.text and entry.text.strip() or ''  
        file_size = len(content)  
        if offset < file_size:  
            if offset + size > file_size:  
                size = file_size - offset  
            return content[offset:offset+size]  
        else:  
            return ''


if __name__ == '__main__':  
    fs = MyFS()  
    fs.parse(errex=1)  
    fs.main() 
