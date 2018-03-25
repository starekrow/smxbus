<?php

// routing module, converts queries to module messages
$cache

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
	if ($r === FALSE) {
		fwrite(STDERR, "Unexpected failure in STDIN");
		die;
	}
	if ($r[0] == "{") {
		// structured value
	} else if ($r[0] == ">") {
		// system/process message
		
	} else if ($r[0] == "!") {
		// error

	} else if ($r[0] == "") {

	} else if ($r[0] == "$") {
		// hex/binary
	} else if (strspn($r[0],"0123456789ABCDEF")) {

	} else if ($r[0] == ";") {
		// text string
		$l = strlen($r);
		$trim = (($r[$l - 2] == "\r") ? 3 : 2);
		$r = substr($r, 1, $l - $trim);
	} else {
		fwrite(STDERR, "Unexpected packet type in STDIN");
		die;
	}
	return $r;
}

function parseCommand( $text )
{

}
