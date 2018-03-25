<?php

// simple module that implements a key-value cache
function writeMsg( $msg )
{
	fwrite(STDOUT, $msg);
}

function sendMsg( $msg )
{
	writeMsg($msg);
	return readMsg();
}

function readMsg()
{
	$r = fgets(STDIN);
	return rtrim($r,"\r\n");
}


function main()
{
	$cache = [];
	$r = sendMsg("init id=hi.starekrow.cache v=0.1");	
	$r = sendMsg("listen");
	for(;;) {
		$m = readMsg();
		$m = explode(" ",$m,4);
		switch ($m[0]) {
		case "put":
			$cache[$m[1]] = [time() + $m[2], $m[3]];
			break;

		case "get":
			if (isset($cache[$m[1]]) && time() >= $cache[$m[1]][0]) {
				writeMsg("ok $data\n");
			} else {
				unset($cache[$m[1]]);
				writeMsg("no\n");
			}
			break;

		case "clr":
			unset($cache[$m[1]]);
			writeMsg("ok\n");
			break;

		case "has":
			if (isset($cache[$m[1]]) && time() >= $cache[$m[1]][0]) {
				writeMsg("ok\n");
			} else {
				unset($cache[$m[1]]);
				writeMsg("no\n");
			}
			break;

		case ">stop":
			sendMsg("ok\n");
			break 2;

		default:
			fwrite(STDERR, "Ignoring unknown message $m[0]");
			writeMsg("no\n");
		}
	}
}

