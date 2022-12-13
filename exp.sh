rm -rf ~/dataTemp
rm -rf ~/shinato1/* #dataファイルの下を削除する
##mkdir ~/"shinato2/GPSR"
##mkdir ~/"shinato2/LGPSR"

start_time=`date +%s` 

i=1 #loop
r=10 #実験回数

for protocol in GPSR 
do

	mkdir ~/"dataTemp"
	mkdir ~/"shinato1/$protocol"
	while [ $i -le $r ]; do
	
	echo "-run $i  --RoutingProtocol=$protocol "
	./waf --run "shinato --protocolName=$protocol --nodeNum=60 -simTime=430 "
	
	mv ~/dataTemp/data.txt ~/dataTemp/data$i.txt
	mv ~/dataTemp/data$i.txt ~/shinato1/$protocol
	i=`expr $i + 1`
	
	done
	
	i=1
	rm -rf ~/dataTemp	

done

./shinatomaketable $r

end_time=`date +%s` #unix時刻から現在の時刻までの秒数を取得

  SS=`expr ${end_time} - ${start_time}` #シュミレーションにかかった時間を計算する　秒数
  HH=`expr ${SS} / 3600` #時を計算
  SS=`expr ${SS} % 3600`
  MM=`expr ${SS} / 60` #分を計算
  SS=`expr ${SS} % 60` #秒を計算

  echo "シュミレーション時間${HH}:${MM}:${SS}" #シミュレーションにかかった時間を　時:分:秒で表示する


