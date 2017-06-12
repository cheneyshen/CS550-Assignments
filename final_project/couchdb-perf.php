<?php

$config = array(
        'method' => 'single_insert',
        'insertCounts' => array(
                100000,
        )
);

$db = new CouchDb();
foreach ($config['insertCounts'] as $docCount) {
        // Re-create the database for each attempt

        for ($i = 0; $i < 16; $i++)
        {
                $db->send($i, 'delete', '/benchmark_db');
                $db->send($i, 'put', '/benchmark_db');
        }

        echo sprintf("-> %s %d docs:\n", $config['method'], $docCount);
        $maxInsert = $maxFind = $maxDelete = 0;
        $minInsert = $minFind = $minDelete = 10000;
        $totalInsert = $totalFind = $totalDelete = 0;
        switch ($config['method']) {
                case 'bulk_insert':
                        $insertStart = microtime(true);
                        $docsWritten = 0;
                        while ($docsWritten < $docCount) {
                                $insertAtOnce = ($docCount - $docsWritten > 1000)
                                        ? 1000
                                        : $docCount - $docsWritten;

                                $docs = array();
                                for ($i = 0; $i < $insertAtOnce; $i++) {
                                        $docs[] = array(
                                                '_id' => CouchDb::uuid(),
                                                'foo' => 'bar'
                                        );
                                }
                                $db->send(rand(0,7), 'post', '/benchmark_db/_bulk_docs',  compact('docs'));
                                $docsWritten = $docsWritten + $insertAtOnce;
                                echo '.';
                        }
                        $insertEnd = microtime(true);
                        break;
                case 'single_insert':
                        $temp = 'valuevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevalue';
                        for ($i = 1000000000; $i < 1000000000 + $docCount; $i++) {
                                $key = (string)$i;
                                $value = $temp.$key;
                                $serial = rand(0,15);
                                $insertStart = microtime(true);//CouchDb::uuid()
                                $db->send($serial, 'put', sprintf('/benchmark_db/%s', $i), array($key=>$value));
                                $insertEnd = microtime(true);
                                $currentInsert = $insertEnd - $insertStart;
                                if ($maxInsert < $currentInsert)
                                        $maxInsert = $currentInsert;
                                if ($minInsert > $currentInsert)
                                        $minInsert = $currentInsert;
                                $totalInsert += $currentInsert;
                                $findStart = microtime(true);
                                $resp = $db->send($serial, 'get', sprintf('/benchmark_db/%s', $i) );
                                $findEnd = microtime(true);
                                $currentFind = $findEnd - $findStart;
                                if ($maxFind < $currentFind)
                                        $maxFind = $currentFind;
                                if ($minFind > $currentFind)
                                        $minFind = $currentFind;
                                $totalFind += $currentFind;
                                $deleteStart = microtime(true);
                                $resp = $db->send($serial, 'delete', sprintf('/benchmark_db/%s?rev=%s', $i, $resp["_rev"]));
                                $deleteEnd = microtime(true);
                                $currentDelete = $deleteEnd - $deleteStart;
                                if ($maxDelete < $currentDelete)
                                        $maxDelete = $currentDelete;
                                if ($minDelete > $currentDelete)
                                        $minDelete = $currentDelete;
                                $totalDelete += $currentDelete;
                        }
                        break;
        }

        clearstatcache();
/*
        $beforeCompact = array(
                'stats' => $db->send('get', '/benchmark_db'),
        );


        $compactStart = microtime(true);
        $r = $db->send('post', '/benchmark_db/_compact');
        while ($status = $db->send('get', '/benchmark_db')) {
                if (!$status['compact_running']) {
                        break;
                }
                echo 'x';
                usleep(1000000);
        }
        $compactEnd = microtime(true);

        clearstatcache();
        $afterCompact = array(
                'stats' => $db->send('get', '/benchmark_db'),
        );
*/
        //echo "\n\n";
        echo sprintf(
                "insert 10K time: %s sec\n".
                "insert time / doc: %s ms\n".
                "insert max time: %s ms\n".
                "insert min time: %s ms\n".
                "find 10K time: %s sec\n".
                "find time / doc: %s ms\n".
                "find max time: %s ms\n".
                "find min time: %s ms\n".
                "delete 10K time: %s sec\n".
                "delete time / doc: %s ms\n".
                "delete max time: %s ms\n",
                "delete min time: %s ms\n",
                round($totalInsert, 4),
                round(($totalInsert * 1000) / $docCount, 3),
                round($maxInsert * 1000, 3),
                round($minInsert * 1000, 3),
                round($totalFind, 4),
                round(($totalFind * 1000) / $docCount, 3),
                round($maxFind * 1000, 3),
                round($minFind * 1000, 3),
                round($totalDelete, 4),
                round(($totalDelete * 1000) / $docCount, 3),
                round($maxDelete * 1000, 3)
                round($minDelete * 1000, 3)
        );
}

class CouchDb {
        public $config = array(
                array(
                'host' => '40.117.103.147',
                'port' => 5984),
                array(
                'host' => '40.121.91.161',
                'port' => 5984),
                array(
                'host' => '40.117.94.67',
                'port' => 5984),
                array(
                'host' => '40.121.91.83',
                'port' => 5984),
                array(
                'host' => '40.121.95.5',
                'port' => 5984),
                array(
                'host' => '40.117.91.244',
                'port' => 5984),
                array(
                'host' => '104.45.139.6',
                'port' => 5984),
                array(
                'host' => '40.114.7.144',
                'port' => 5984),
                array(
                'host' => '104.41.145.114',
                'port' => 5984),
                array(
                'host' => '40.117.90.161',
                'port' => 5984),
                array(
                'host' => '40.117.90.71',
                'port' => 5984),
                array(
                'host' => '40.117.92.166',
                'port' => 5984),
                array(
                'host' => '40.117.91.155',
                'port' => 5984),
                array(
                'host' => '40.121.91.190',
                'port' => 5984),
                array(
                'host' => '40.117.88.60',
                'port' => 5984),
                array(
                'host' => '40.117.94.255',
                'port' => 5984),
        );

        public function __construct($config = array()) {
                $this->config = $config + $this->config;
        }

        public function send($serial, $method, $resource, $document = array()) {
                $url = sprintf(
                        'http://%s:%s%s',
                        $this->config[$serial]['host'],
                        $this->config[$serial]['port'],
                        $resource
                );
		
                $curlOptions = array(
                        CURLOPT_RETURNTRANSFER => true,
                        CURLOPT_CUSTOMREQUEST => strtoupper($method),
                );

                if (!empty($document)) {
                        $curlOptions[CURLOPT_POSTFIELDS] = json_encode($document);
                }


                $curl = curl_init($url);
                curl_setopt_array($curl, $curlOptions);
                $r = curl_exec($curl);

                return json_decode($r, true);
        }

        public static function uuid() {
                return substr(sha1(uniqid(mt_rand(), true)), 0, 32);

        }
}

?>
