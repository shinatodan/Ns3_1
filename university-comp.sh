rm -rf ~/dataTemp
rm -rf ~/University/* #dataファイルの下を削除する
mkdir ~/"University/AODV"
mkdir ~/"University/OLSR"
mkdir ~/"University/GPSR"
mkdir ~/"University/IGPSR"
mkdir ~/"University/PGPSR"
mkdir ~/"University/LGPSR"

start_time=`date +%s` 

i=1 #loop
r=30 #実験回数

for protocol in AODV OLSR GPSR IGPSR PGPSR LGPSR
do

	mkdir ~/"dataTemp"
	while [ $i -le $r ]; do
	
	echo "-run $i  --RoutingProtocol=$protocol "
	./waf --run "university --protocolName=$protocol "
	
	mv ~/dataTemp/data.txt ~/dataTemp/data$i.txt
	mv ~/dataTemp/data$i.txt ~/University/$protocol
	i=`expr $i + 1`
	
	done
	
	i=1
	rm -rf ~/dataTemp	

done

./Universitymaketable $r

end_time=`date +%s` #unix時刻から現在の時刻までの秒数を取得

  SS=`expr ${end_time} - ${start_time}` #シュミレーションにかかった時間を計算する　秒数
  HH=`expr ${SS} / 3600` #時を計算
  SS=`expr ${SS} % 3600`
  MM=`expr ${SS} / 60` #分を計算
  SS=`expr ${SS} % 60` #秒を計算

  echo "シュミレーション時間${HH}:${MM}:${SS}" #シミュレーションにかかった時間を　時:分:秒で表示する
