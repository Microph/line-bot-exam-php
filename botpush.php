<?php

require "vendor/autoload.php";

$messagingType = "";
$replyToken = "";
foreach ($_POST as $key => $value){
   //file_put_contents("php://stderr", "post text from arduino key: " . $key . "\nvalue: " . $value . "\n");
   if($key == "messagingType")
   {
      $messagingType = $value;
   }
   else if($key == "userID")
   {
      $userID = $value;
   }
   else if($key == "replyToken")
   {
      $replyToken = $value;
   }
   else if($key == "text")
   { 
      if($messagingType == "reply")
      {
         send_reply_message($value, $replyToken);
      }
      else
      {
         send_push_message($value, $userID);
      }
   }
}

//Can only send to one specific account -> need database implementation to store pushIDs
function send_push_message($msg, $pushID){
   $access_token = 'gfFXBM/EbegiS1D3eWlCV64GBADykoNE8YxuDepBcp1+YSqUcdYv0HU8Afzr2rq+qkBrbXF3h+auDA/qRS60wKIN4pcX+bGoc5FQOUWoERdjEHMOpljuKpEh1e2VmocNghJwLR9W9C4yQIHMc8xsKwdB04t89/1O/w1cDnyilFU=';
   $channelSecret = '5a4a6b9b0cf3415b8071f4ffbe08fe3d';

   $httpClient = new \LINE\LINEBot\HTTPClient\CurlHTTPClient($access_token);
   $bot = new \LINE\LINEBot($httpClient, ['channelSecret' => $channelSecret]);

   $textMessageBuilder = new \LINE\LINEBot\MessageBuilder\TextMessageBuilder($msg);
   $response = $bot->pushMessage($pushID, $textMessageBuilder);

   file_put_contents("php://stderr", "push message result: " . $response->getHTTPStatus() . ' ' . $response->getRawBody());
}

function send_reply_message($msg, $replyToken){
   $access_token = 'gfFXBM/EbegiS1D3eWlCV64GBADykoNE8YxuDepBcp1+YSqUcdYv0HU8Afzr2rq+qkBrbXF3h+auDA/qRS60wKIN4pcX+bGoc5FQOUWoERdjEHMOpljuKpEh1e2VmocNghJwLR9W9C4yQIHMc8xsKwdB04t89/1O/w1cDnyilFU=';
   $messages = 
   [	
      'type' => 'text',	
      'text' => $msg	
   ];
   // Make a POST Request to Messaging API to reply to sender
   $url = 'https://api.line.me/v2/bot/message/reply';
   $data = [
      'replyToken' => $replyToken,
      'messages' => [$messages],
   ];
   $post = json_encode($data);
   $headers = array('Content-Type: application/json', 'Authorization: Bearer ' . $access_token);

   $ch = curl_init($url);
   curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
   curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
   curl_setopt($ch, CURLOPT_POSTFIELDS, $post);
   curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
   curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
   $result = curl_exec($ch);
   file_put_contents("php://stderr", "reply result: " . $result . "\r\n");
   curl_close($ch);
}
