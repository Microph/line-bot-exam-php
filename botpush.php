<?php

require "vendor/autoload.php";

foreach ($_REQUEST as $key => $value){
   file_put_contents("php://stderr", "post text from esp8266 key: " . $key . " value: " . $value);
}

function send_push_message($msg){

    $access_token = 'gfFXBM/EbegiS1D3eWlCV64GBADykoNE8YxuDepBcp1+YSqUcdYv0HU8Afzr2rq+qkBrbXF3h+auDA/qRS60wKIN4pcX+bGoc5FQOUWoERdjEHMOpljuKpEh1e2VmocNghJwLR9W9C4yQIHMc8xsKwdB04t89/1O/w1cDnyilFU=';

    $channelSecret = '5a4a6b9b0cf3415b8071f4ffbe08fe3d';

    $pushID = 'Ud168d219c6397f81416451f5ffc0b081';

    $httpClient = new \LINE\LINEBot\HTTPClient\CurlHTTPClient($access_token);
    $bot = new \LINE\LINEBot($httpClient, ['channelSecret' => $channelSecret]);

    $textMessageBuilder = new \LINE\LINEBot\MessageBuilder\TextMessageBuilder($msg);
    $response = $bot->pushMessage($pushID, $textMessageBuilder);

    file_put_contents("php://stderr", "push message result: " . $response->getHTTPStatus() . ' ' . $response->getRawBody());

}

