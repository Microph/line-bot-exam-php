<?php

 function pubMqtt($topic,$msg){
   $APPID= "SitTooLong/"; //enter your appid
   $KEY = "Hwmr3uwcLINjDXk"; //enter your key
   $SECRET = "MKCEEVxfFkc6y3f4iXTV2DbB9"; //enter your secret
   $Topic = "$topic";
   put("https://api.netpie.io/microgear/".$APPID.$Topic."?retain&auth=".$KEY.":".$SECRET,$msg);
 }

 function getMqttfromlineMsg($topic,$lineMsg){
    pubMqtt($topic,$lineMsg);
  }
 
  function put($url,$tmsg){
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 10);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
    curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_setopt($ch, CURLOPT_POSTFIELDS, $tmsg);
    $response = curl_exec($ch);

    curl_close($ch);
    echo $response . "\r\n";
    return $response;
  }
?>
