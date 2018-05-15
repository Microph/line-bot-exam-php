<?php // callback.php

require "vendor/autoload.php";
require ("botpush.php");
require ("pub.php");
require_once('vendor/linecorp/line-bot-sdk/line-bot-sdk-tiny/LINEBotTiny.php');

// Get POST body content
$content = file_get_contents('php://input');
// Parse JSON
$events = json_decode($content, true);
// Validate parsed JSON data
if (!is_null($events['events'])) {
	// Loop through each event
	foreach ($events['events'] as $event) {
		// Reply only when message sent is in 'text' format
		if ($event['type'] == 'message' && $event['message']['type'] == 'text') {
			// Get user ID
			$userID = $event['source']['userId'];
			// Get replyToken
			$replyToken = $event['replyToken'];
			
			$text = $event['message']['text'];
			$Topic = "NodeMCU1" ;
			getMqttfromlineMsg($Topic,$text);
		}
	}
}
echo "OK";
