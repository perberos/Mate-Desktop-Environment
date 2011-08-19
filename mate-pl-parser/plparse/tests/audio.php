<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
<META name="description" content="Ouça o melhor da MPB na Nova Brasil FM" />
<title>Rádio Ao Vivo | Rede Nova Brasil FM - Moderna e Brasileira</title>
<style type="text/css">
<!--
body {
	margin-left: 0px;
	margin-top: 0px;
	margin-right: 0px;
	margin-bottom: 0px;
	background:url(../images/bg_player01.jpg) no-repeat;
	font-family:Georgia, "Times New Roman", Times, serif;
	font-size:11px; letter-spacing:-1px;
}
#fundo {
	width:300px;
	height:362px;
}
.titulo { color:#3e6489; }
.texto { color:#3e643e; }
.letra { margin-top:10px; font-weight:bold; color:#000033; display:block; text-decoration:none }
-->
</style>
<script language="javascript">window.resizeTo(310, 527);</script>
<script type="text/javascript" src="/scripts/jQuery/jquery-1.2.6.min.js"></script>
<script type="text/javascript">
$(function() {
	setInterval('refresh_music()', 30000);
	refresh_music();
});
function refresh_music () {
	$('#dados_musica').load("/ao-vivo/atualiza-dados.php");
}
</script>
<script type="text/javascript" src="http://partner.googleadservices.com/gampad/google_service.js">
</script>
<script type="text/javascript">
  GS_googleAddAdSenseService("ca-pub-7708583469693487");
  GS_googleEnableAllServices();
</script>
<script type="text/javascript">
  GA_googleAddSlot("ca-pub-7708583469693487", "player_rodape_234x60");
</script>
<script type="text/javascript">
  GA_googleFetchAds();
</script>
<script type="text/javascript">var _sf_startpt=(new Date()).getTime()</script>
</head>

<body>
<table width="300" height="362" border="0" cellspacing="0" cellpadding="0">
  <tr>
    <td id="fundo" valign="bottom" style="width:273px; height:295px; *height:280px; padding:0 0 15px 27px; *padding:0 0 21px 27px;"><object
			id="MediaPlayer"
			name="MediaPlayer"
			border=0
			width=247
			height=63
			classid="clsid:22D6F312-B0F6-11D0-94AB-0080C74C7E95" codebase="http://activex.microsoft.com/activex/controls/mplayer/en/nsmp2inf.cab#Version=6,4,5,715" standby="Loading Microsoft Windows Media Player components..." type="application/x-oleobject" VIEWASTEXT>
			<param name="FileName" value="http://00086.cdn.upx.net.br/listen.wmx">
			<param name="autoStart" value="True">
            <param name="uiMode" value="Mini">
            <param name="ShowControls" value="True">
            <param name="ShowStatusBar" value="True">
			<embed type="application/x-mplayer2" pluginspage="http://www.microsoft.com/windows/mediaplayer/download/default.asp" filename="http://00086.cdn.upx.net.br/listen.wmx" uimode="mini" src="http://00086.cdn.upx.net.br/listen.wmx" showstatusbar="1" autostart="1" showcontrols="0" width="247" height="63" align="texttop"> </embed>
		</object></td>
  </tr>
  <tr>
  <td height="55" style="padding:0 35px;" valign="middle" id="dados_musica"></td>
  </tr>
  <tr>
    <td height="103" align="center">
	<script type="text/javascript">
	  GA_googleFillSlot("player_rodape_234x60");
	</script>
    </td>
  </tr>
</table>
<script type="text/javascript">
var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." :
"http://www.");
document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E"));
</script>
<script type="text/javascript">
var pageTracker = _gat._getTracker("UA-689102-5"); pageTracker._trackPageview(); </script>
<script type="text/javascript">
var cbjspath = "static.chartbeat.com/js/chartbeat.js?uid=1709&domain=novabrasilfm.com.br";
var cbjsprotocol = (("https:" == document.location.protocol) ? "https://s3.amazonaws.com/" : "http://");
document.write(unescape("%3Cscript src='"+cbjsprotocol+cbjspath+"' type='text/javascript'%3E%3C/script%3E"))
</script>
</body>
</html>
