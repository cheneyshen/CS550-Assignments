#!/usr/bin/env python
# -*- coding: utf-8 -*-

from cassandra.cluster import Cluster
import logging
import datetime
import time
import random

log = logging.getLogger()
log.setLevel('INFO')

maxWrite = datetime.timedelta(0)
minWrite = datetime.timedelta(10000)
maxFind = datetime.timedelta(0)
minFind = datetime.timedelta(10000)
maxDelete = datetime.timedelta(0)
minDelete = datetime.timedelta(10000)
totalWrite = datetime.timedelta(0)
totalFind = datetime.timedelta(0)
totalDelete = datetime.timedelta(0)

class SimpleClient(object):
    session = None
    
    def connect(self, nodes):
        cluster = Cluster(nodes, protocol_version=2)
        metadata = cluster.metadata
        self.session = cluster.connect()
        log.info('Connected to cluster: ' + metadata.cluster_name)
        for host in metadata.all_hosts():
            log.info('Datacenter: %s; Host: %s; Rack: %s',
                host.datacenter, host.address, host.rack)

    def close(self):
        self.session.execute("""DROP KEYSPACE simplex; """)
        self.session.cluster.shutdown()
        log.info('Connection closed.')


    def create_schema(self):
        try:
            self.session.execute("""CREATE KEYSPACE simplex WITH replication = {'class':'SimpleStrategy', 'replication_factor':3};""")
        except Exception, e:
            x = 1
        try:
            self.session.execute("""
                CREATE TABLE simplex.songs (
                id int PRIMARY KEY,
                title text
                );
            """)
        except Exception, e:
            x = 2
        log.info('Simplex keyspace and schema created.')


    def execute_data(self, i):
    	global maxWrite
    	global maxFind
    	global maxDelete
    	global minWrite
    	global minFind
    	global minDelete
    	global totalWrite
    	global totalFind
    	global totalDelete
        insert_song_prepared_statement = self.session.prepare(
         """
         INSERT INTO simplex.songs
         (id, title)
         VALUES (?, ?);
         """)
        query_song_prepared_statement = self.session.prepare(
         """
         SELECT * FROM simplex.songs
         where (id = ?);
         """)
        delete_song_prepared_statement = self.session.prepare(
         """
         DELETE FROM simplex.songs
         where (id = ?);
         """)
        i += 1000000000
        key = str(i)
        value = "valuevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevalue" + str(i)
        done1 = datetime.datetime.now()
        self.session.execute(insert_song_prepared_statement, [i, value])
        done2 = datetime.datetime.now()
        write = done2 - done1
        totalWrite += write
        if (maxWrite < write):
            maxWrite = write
        if (minWrite > write):
            minWrite = write
        done1 = datetime.datetime.now()
        self.session.execute(query_song_prepared_statement, [i])
        done2 = datetime.datetime.now()
        find = done2 - done1
        totalFind += find
        if (maxFind < find):
            maxFind = find
        if (minFind > find):
            minFind = find
        done1 = datetime.datetime.now()
        self.session.execute(delete_song_prepared_statement, [i])
        done2 = datetime.datetime.now()
        delete = done2 - done1
        totalDelete += delete
        if (maxDelete < delete):
            maxDelete = delete
        if (minDelete > delete):
            minDelete = delete
        #log.info('Data executed.')

def main():
    logging.basicConfig()
    client = []
    for i in range(16):
        client.append(SimpleClient())
    client[0].connect(['40.121.91.83'])
    client[1].connect(['40.121.91.161'])
    client[2].connect(['40.121.95.5'])
    client[3].connect(['40.117.94.67'])
    client[4].connect(['40.117.103.147'])
    client[5].connect(['40.117.90.71'])
    client[6].connect(['104.41.145.114'])
    client[7].connect(['40.121.91.190'])
    client[8].connect(['104.45.139.6'])
    client[9].connect(['40.114.7.144'])
    client[10].connect(['40.117.92.166'])
    client[11].connect(['40.117.91.244'])
    client[12].connect(['40.117.90.161'])
    client[13].connect(['40.117.88.60'])
    client[14].connect(['40.117.91.155'])
    client[15].connect(['40.117.94.255'])

    for i in range(16):
        client[i].create_schema()
    time.sleep(10)
    for i in range(100000):
        dest = random.randint(0, 15)
        client[dest].execute_data(i)
    for i in range(16):
        client[i].close()
    print maxWrite
    print minWrite
    print maxFind
    print minFind
    print maxDelete
    print minDelete
    print totalWrite
    print totalFind
    print totalDelete

if __name__ == "__main__":
    main()