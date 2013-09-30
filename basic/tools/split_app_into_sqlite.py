#!/usr/bin/python 
#-*- coding: utf8 -*- 

import os
import os.path
import sys
import uuid

def print_usage():
	print '***********************************************************************'
	print 'Usage:', sys.argv[0], ' sqlite_db target_file version_num prescr_file postscr_file'
	print 'Warning: the PATH of the target file and'
	print '         database should use absolutely path'
	print '***********************************************************************'

import hashlib
def md5_for_file(path, hr=False, block_size=256*128):
    '''
    Block size directly depends on the block size of your filesystem
    to avoid performances issues
    Here I have blocks of 4096 octets (Default NTFS)
    '''
    md5 = hashlib.md5()
    with open(path,'rb') as f: 
        for chunk in iter(lambda: f.read(block_size), b''): 
             md5.update(chunk)
    if hr:
        return md5.hexdigest()
    return md5.digest()


if (len(sys.argv) != 6):
	print_usage()
	exit(1)

version_num = str(sys.argv[3])
tar_file    = str(sys.argv[2])
sqlite_db   = str(sys.argv[1])

preinstallscript   = ""
postinstallscript  = "" 
with open(sys.argv[4], 'r') as scr_f:
	preinstallscript = scr_f.read()

with open(sys.argv[5], 'r') as scr_f:
	postinstallscript = scr_f.read()

split_size   = 2048
package_name = os.path.basename(tar_file)
package_size = os.path.getsize(tar_file)
md5sum       = md5_for_file(tar_file, True)
print 'Package_name:', package_name, 'Package_size:', package_size, 'MD5:', md5sum


#check the time usage
import time
start = int(round(time.time() * 1000))

import sqlite3
tb_name_packagelist = 'tblsoftwarepackagelist'
tb_name_package     = 'tblsoftwarepackage'

db_conn     = sqlite3.connect(sqlite_db)

# Enabling Foreign Key Support
#db_conn.execute('PRAGMA foreign_keys = OFF')
#db_conn.commit()

# Start insert
insert_str  = '''INSERT INTO %s(guid, packagename, versionnumber, packagesize, checksum, preinstallscript, postinstallscript)
				VALUES('%s', '%s', '%s', %d, '%s', '%s', '%s')''' % \
				(tb_name_packagelist, str(uuid.uuid4()).strip("-"), package_name, version_num, package_size, md5sum, preinstallscript, postinstallscript)
print 'Start writing into', tb_name_packagelist, 'table:', insert_str
start = int(round(time.time() * 1000))
db_conn.execute(insert_str)
db_conn.commit()
end = int(round(time.time() * 1000))
print 'One insert using(millisecond):', end-start

select_str  = "SELECT guid FROM %s WHERE checksum='%s'" % (tb_name_packagelist, md5sum)
packageguid = db_conn.execute(select_str).fetchone()[0]
print 'Get packageguid=', packageguid

start = int(round(time.time() * 1000))
serialno    = 0
read_times  = 2*1024
with open(tar_file, 'rb') as outfile:
	try:
		db_conn.execute('BEGIN')
		while True:
 			# write into database
 			content=outfile.read(split_size*read_times)
 			read_len = len(content)
 			if read_len == 0:
 				break;
			write_time = (read_len/split_size) if (read_len%split_size == 0) else (read_len/split_size+1)
			print 'write_time=', write_time
			for i in range(0, write_time):
				sys.stdout.write('.');sys.stdout.flush()
				serialno += 1
				insert_str = '''INSERT INTO %s(guid, packageid, serialno, content)
		   	          VALUES('%s', '%s', %d, ?)''' % (tb_name_package,  str(uuid.uuid4()).strip('-'), packageguid, serialno)
				db_conn.execute(insert_str, [buffer(content, i*split_size, split_size)])
		db_conn.commit()
	except sqlite3.Error, e:
		if db_conn:
			db_conn.rollback()
		print "Error %s:" % e.args[0]
		sys.exit(1)
	finally:	
		db_conn.close()
		print ''
		end = int(round(time.time() * 1000))
		print 'The time usage(millisecond):', end-start

































