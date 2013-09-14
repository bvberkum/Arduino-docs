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

DEFAULT_NODES = [ '.', '..' ] 
def now():
    return int(time.time())  

class MyFS(fuse.Fuse):  

    tree = {
            'nodes': {'text': "Test"},
            'values': {'text': "Test"},
            'logs': {'text': "Test"},
        }

    def __init__(self, *args, **kw):  
        fuse.Fuse.__init__(self, *args, **kw)  

    def getattr(self, path):
        st = fuse.Stat()  
        st.st_mode = stat.S_IFDIR | 0755  
        st.st_nlink = 2  
        st.st_atime = now()
        st.st_mtime = st.st_atime  
        st.st_ctime = st.st_atime  
        path = path.strip('/')
        if path == '':  
            pass  
        elif path in self.tree.keys() or path in DEFAULT_NODES:
            pass
        else:  
            return - errno.ENOENT  
        return st  

    def readdir(self, path, offset):
        entry = self.tree.find(path)  
        if hasattr(entry, 'text'):
            return '' # TODO: proper error
        path = path.strip('/')
        if path == '':
            for node in DEFAULT_NODES + self.tree.keys():
                yield fuse.Direntry(node)
        elif path == 'nodes':
            for node in DEFAULT_NODES:
                yield fuse.Direntry(node)
        elif path == 'values':
            for node in DEFAULT_NODES:
                yield fuse.Direntry(node)
        elif path == 'logs':
            for node in DEFAULT_NODES:
                yield fuse.Direntry(node)

    def open(self, path, flags):
        entry = self.tree.find(path)  
        if hasattr(entry, 'text'):
            return '' # TODO: proper error
        #
        access_flags = os.O_RDONLY | os.O_WRONLY | os.O_RDWR
        if flags & access_flags != os.O_RDONLY:
            return -errno.EACCES
        else:
            return 0

    def read(self, path, size, offset):
        entry = self.tree.find(path)  
        if hasattr(entry, 'text'):
            return '' # TODO: proper error
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
