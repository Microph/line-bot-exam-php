<?php
$access_token = 'gfFXBM/EbegiS1D3eWlCV64GBADykoNE8YxuDepBcp1+YSqUcdYv0HU8Afzr2rq+qkBrbXF3h+auDA/qRS60wKIN4pcX+bGoc5FQOUWoERdjEHMOpljuKpEh1e2VmocNghJwLR9W9C4yQIHMc8xsKwdB04t89/1O/w1cDnyilFU=';


$url = 'https://api.line.me/v1/oauth/verify';

$headers = array('Authorization: Bearer ' . $access_token);

$ch = curl_init($url);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
$result = curl_exec($ch);
curl_close($ch);

echo $result;
