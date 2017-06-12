#!/usr/bin/env python

import pymongo
import datetime
import random
connection = []
db = []
connection.append(pymongo.MongoClient('40.117.103.147',27017))
db.append(connection[0].test)
connection.append(pymongo.MongoClient('40.121.91.161',27017))
db.append(connection[1].test)
connection.append(pymongo.MongoClient('40.117.94.67',27017))
db.append(connection[2].test)
connection.append(pymongo.MongoClient('40.121.91.83',27017))
db.append(connection[3].test)
connection.append(pymongo.MongoClient('40.121.95.5',27017))
db.append(connection[4].test)
connection.append(pymongo.MongoClient('40.117.91.244',27017))
db.append(connection[5].test)
connection.append(pymongo.MongoClient('104.45.139.6',27017))
db.append(connection[6].test)
connection.append(pymongo.MongoClient('40.114.7.144',27017))
db.append(connection[7].test)
connection.append(pymongo.MongoClient('104.41.145.114',27017))
db.append(connection[8].test)
connection.append(pymongo.MongoClient('40.117.90.161',27017))
db.append(connection[9].test)
connection.append(pymongo.MongoClient('40.117.90.71',27017))
db.append(connection[10].test)
connection.append(pymongo.MongoClient('40.117.92.166',27017))
db.append(connection[11].test)
connection.append(pymongo.MongoClient('40.117.91.155',27017))
db.append(connection[12].test)
connection.append(pymongo.MongoClient('40.121.91.190',27017))
db.append(connection[13].test)
connection.append(pymongo.MongoClient('40.117.88.60',27017))
db.append(connection[14].test)
connection.append(pymongo.MongoClient('40.117.94.255',27017))
db.append(connection[15].test)

maxWrite = datetime.timedelta(0)
minWrite = datetime.timedelta(10000)
maxFind = datetime.timedelta(0)
minFind = datetime.timedelta(10000)
maxDelete = datetime.timedelta(0)
totalWrite = datetime.timedelta(0)
totalFind = datetime.timedelta(0)
totalDelete = datetime.timedelta(0)
minDelete = datetime.timedelta(10000)
for i in range(100000):
	dest = random.randint(0, 15)
	i += 1000000000
	value = "valuevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevalue"
	post = {str(i): value + str(i)}
	done1 = datetime.datetime.now()
	db[dest].foo.insert(post)
	done2 = datetime.datetime.now()
	write = done2 - done1
	totalWrite += write
	if (maxWrite < write):
			maxWrite = write
	if (minWrite > write):
			minWrite = write
	done1 = datetime.datetime.now()
	db[dest].foo.find_one(post)
	done2 = datetime.datetime.now()
	find = done2 - done1
	totalFind += find
	if (maxFind < find):
			maxFind = find
	if (minFind > find):
			minFind = find
	done1 = datetime.datetime.now()
	db[dest].foo.remove(post)
	done2 = datetime.datetime.now()
	delete = done2 - done1
	totalDelete += delete
	if (maxDelete < delete):
			maxDelete = delete
	if (minDelete > delete):
			minDelete = delete
print maxWrite
print minWrite
print maxFind
print minFind
print maxDelete
print minDelete
print totalWrite
print totalFind
print totalDelete