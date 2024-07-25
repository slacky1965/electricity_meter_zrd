# Electricity Meter Zigbee TLSR8258 (E-Byte E180-Z5812SP)

## Устройство для дистанционного мониторинга многотарифных однофазных счетчиков с последующей передачей показаний в Home Assistant.

**Включает в себя схему оптопорта и модуль от E-Byte E180-Z5812SP, который работает, как Zigbee-роутер**

# Not finished! In the process! Описание временное, проект в разработке ...

[Repository electricity_meter_zrd](https://github.com/slacky1965/electricity_meter_zrd/tree/version1)  

---

# Внимание!!! Плата пришла и протестированна!!! Есть ошибка в схеме!!!

Не подведено питание в датчику температуры. 
	
<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/ds18b20_wo_vcc.jpg"/>

Решается обычной перемычкой из МГТФ. 
	
<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/board/board_top_wire.jpg"/>
	
Все остальное работает.
	
# Но! Схема исправлена. Плата обновлена. Но не заказана и не протестирована :))

---

* [Описание](#description)
* [Железо](#hardware)
* [Компиляция](#compilation)  
* [Загрузка прошивки](#firmware_download)
* [Принцип работы](#firmware)
* [Настройка](#settings)
* [Приборы учета](#electricity_meters)
* [Home Assistant](#home_assistant)

---

## <a id="description">Описание</a>

* Рассчитано на взаимодействие через оптопорт с однофазными многотарифными электросчетчиками:

	- [Каскад-1МТ](#kaskad-1-mt)	
	- [Каскад-11](#kaskad-11) (требует более глубокого тестирования, нет счетчика под рукой)  
	- [Меркурий-206](#mercury-206)
	- [Энергомера-СЕ102М](#energomera-ce102m)
	- [Нева-МТ124](#neva-mt124)
	- [Нартис-100](#nartis-100)

* Модуль посылает команды электросчетчику и принимает ответы от него. В настоящий момент устройство может прочитать

	- 4 тарифа (в kWh)
	- силу тока (если доступно, в A)
	- напряжение сети (если доступно, в V)
	- мощность (если доступно, в kW)
	- оставшийся ресурс батарии прибора (если доступно, в %)
	- полный серийный номер прибора (например 3171112520109)
	- дату изготовления прибора (если доступно, например 04.10.2017)

* Сохраняет в энергонезависимой памяти модуля только конфигурационные данные.
* Передает показания по сети Zigbee.
* Взаимодейстивие с "умными домами" через zigbee2mqtt.
* Первоначальная настройка происходит через web-интерфейс zigbee2mqtt.
* Сбросить устройство до заводских значений (zigbee) - нажать кнопку 5 раз.
* Сделать restart модуля - зажать кнопку более чем на 5 секунд.

---

## <a id="hardware">Железо</a>

В проекте используется модуль от компании E-BYTE на чипе TLSR8258F512ET32 - E180-Z5812SP.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/E180-Z5812SP.jpg"/>

Испытывалось все на вот таком dongle от Telink

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/telink_tlsr8258_dongle.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/prototype.jpg"/>

**Схема**

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/Schematic_Electricity_Meter_zrd.jpg"/>

**Плата**

Новая плата разведена, но не заказана и не протестирована.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/board/new_board_top.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/board/new_board_bottom.jpg"/>

На гребенку выведены следующие пины модуля

* SWS, GND - для заливки в модуль прошивки
* RST, TX-DBG - на всякий случай, вдруг кому-то пригодится.

[Ссылка на проект в easyeda](https://oshwlab.com/slacky/electricity_meter_zrd_v3)

**Корпус**

В виде корпуса используется [вилка на 220в](https://leroymerlin.ru/product/vilka-uglovaya-s-zazemleniem-16-a-cvet-belyy-81930756/), купленная в Леруа Мерлен.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box1.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box2.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box3.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box4.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box5.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box6.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box7.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box8.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box9.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/box/box10.jpg"/>

**Готовое устройство**

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/board/board_top_real.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/board/board_bottom_real.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/device1.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/device2.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/device3.jpg"/>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/device4.jpg"/>

---

## <a id="compilation">Компиляция</a>

[Скачиваем проект](https://github.com/slacky1965/electricity_meter_zrd)  
Если скачали архивом, разворачиваем в какой-нибудь временной директории. Далее запускаем Eclipse. В левом верхнем углу нажимаем File, в развернувшемся меню выбираем Import

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/eclipse/eclipse1.jpg"/>

Далее выбираем `Existing Project into Workspace` и жмем `Next`.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/eclipse/eclipse2.jpg"/>

В открывшемся окне по очереди выбираем 

1. `Browse` - находим директорию, куда скачали и развернули проект.
2. Ставим галочку в `Projects:` на появившемся проекте.
3. Ставим галочку на `Copy projects into workspace`
4. Жмем `Finish`

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/eclipse/eclipse3.jpg"/>

Все, проект у нас в Эклипсе. 

Далее нужен компилятор и кое-какие утилиты. Компилятор можно скачать у [Telink'a](https://wiki.telink-semi.cn/wiki/IDE-and-Tools/IDE-for-TLSR8-Chips/). Установщик поставит IDE (правленый Эклипс) и компилятор с утилитами. Но мне такой венегрет не нравится, поэтому я пользуюсь обычным Эклипсом и makefile. Так же компилятор есть у [Ai-Thinker](https://github.com/Ai-Thinker-Open/Telink_825X_SDK). Там есть под Windows и под Linux. Так же советую скачать [git bash for windows](https://git-scm.com/download/win). Это позволит писать makefiles, которые будут прекрасно работать и под Windows и под Linux практически без редактирования. Еще понадобится Python, но я думаю, это не проблема. Не забудьте отредактировать в makefile и bootloader.makefile пути к компилятору, если он у вас лежит в другом месте.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/eclipse/eclipse8.jpg"/>

Итак, компиляцию начинаем с `bootloader`. Собираем и прошиваем ([как и чем шить чуть ниже](#firmware_download)). Если модуль пустой, то после прошивки `bootloader'a` модуль просто начнет моргать светодиодом. Это нормально.  

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/eclipse/eclipse4.jpg"/>

Далее компилируем уже саму прошивку и прошиваем.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/eclipse/eclipse6.jpg"/>

Если все прошло без ошибок, то модуль запустится и начнет работу.

И последнее - проект сделан таким образом, что его можно вообще собрать без IDE, обычнам make в командной строке.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/make.jpg"/>

---

## <a id="firmware_download">Загрузка прошивки</a>

Вопрос - как залить прошивку в модуль. Есть несколько вариантов. Самый простой, это приобрести у Telink их фирменный программатор.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/telink_pgm.jpg"/>

Но он неоправдано дорого стоит. Есть другой, более бюджетный вариант. Заказываем модуль TB-04 или [TB-03](http://www.ai-thinker.com/pro_view-89.html) от Ai-Thinker. Почему-то у самого производителя TB-04 не числится. Но на aliexpress их полно. В них применен TLSR8253. Паяем перемычку согласно фото.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/TB-03F-KIT-PGM.gif"/>
<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/TB-04-KIT-wire.jpg"/>

И заливаем прошивку [вот отсюда](https://github.com/pvvx/TLSRPGM/tree/version1/sources/UART2SWire/tb-0x-pgm)

Все, у нас есть недорогой программатор, который может загружать прошивки через SWS в модули от Telink'a. Пин D4 (SWM) программатора нужно соединить с пином SWS программируемого модуля, не забыть объединить земли и подкинуть питание на оба модуля.  
Сама программа-прошивальщик лежит [тут](https://github.com/pvvx/TLSRPGM)

---

## <a id="firmware">Принцип работы</a>

Устройство является роутером, питается от сети ~200 вольт и никогда не спит.  
По умолчанию считывание показаний с электросчетчика происходит один раз в минуту. Изменить в большую сторону можно через интерфейс zigbee2mqtt во вкладке Exposes ([подробней о настройках чуть ниже](#settings)).

**Reporting**

Устройство высылает 12 "явных" отчетов. И 8 "скрытых" из интерфеса пользователя.

***Явные отчеты***

* Температура (1)
* Четыре тарифа (4)
* Остаточный процент жизни батарейки электросчетчика (1)
* Серийный номер электросчетчика (1)
* Дата изготовления электросчетчика (1)
* Наименование электросчетчика (1)
* Напряжение сети (1)
* Сила тока (1)
* Мощность (1)

Настроить периоды отправки отчетов, если не устроят по умолчанию, можно в интерфейсе zigbee2mqtt во вкладке reporting([подробней о настройках чуть ниже](#settings)).

***Скрытые отчеты***

* Множитель и делитель тарифов (2)
* Множитель и делитель для напряжения сети (2)
* Множитель и делитель силы тока (2)
* Множитель и делитель для мощности (2)

Настроить периоды отправки "скрытых" отчетов нельзя. Они отпраляются принудительно перед одноименным значением. Например, изменилось напряжение сети. Перед отправкой отчета со значением напражения сети принудительно высылаются два отчета - множитель и делитель напряжения сети. И т.д. Связано с тем, что правильное значение высчитывается в конверторе и множитель и делитель должны быть уже определены до получения значения. В обычном варианте иногда отчеты по множителю или делителю приходили позже значения. В данной схеме с принудительными отчетами множитель и делитель всегда приходят раньше значения.

**Светодиодная индикация режимов модуля**

Красный светодиод сигнализирует о присутствии питания на модуле. Ну и косвенно говорит нам, что программа запустилась и работает. 
Зеленый светодиод служит для информирования о режимах работы модуля.

* Одна вспышка - модуль в сети, считывание данных с электросчетчика успешно.
* Две вспышки - модуль в сети, считывание данных с электросчетчика не происходит.
* Три вспышки - модуль не в сети, считывание данных с электросчетчика успешно.
* Четыре вспышки - модуль не в сети, считывание данных с электросчетчика не происходит.
	
**Память модуля, прошивка (firmware) и где хранится конфиг**

В данном проекте обновление OTA не используется. Связано это с тем, что при конфигурации с OTA на TLSR8258F512 отведено под прошивку только 0x34000 байт. И даже, если применить модуль TLST8258F1 с 1мБ - это все равно не сильно исправляет ситуацию, в нем отведено место под прошивку 0x40000. К чему все это. Размер прошивки с 4 счетчиками подошел к своему пределу, что-то около 200000 байт. Если предположить, что счетчики будут добавляться, то мы в самом ближайшем будущем столкнемся с тем, что прошивка превысит размер выделенного под нее пространстсва. Раздельная компиляция для конкретного счетчика мне не понравилась, там вылезли свои глюки. Потому было принято решение остановиться на схеме с использованием `bootloader'a`. При таком решении и отключенном обновлением OTA, размер прошивки может достигать более 400 кБ, что достаточно много. Карту памяти можно посмотреть тут - tl_zigbee_sdk/proj/drivers/drv_nv.h.

Согласно спецификации на чип TLSR8258F512ET32 при использовании `bootloader'a` память распределена следующим образом

		0x00000 Bootloader
		0x08000 Firmware
		0x39000 OTA Image
		0x6A000 NV_1
		0x76000 MAC address
		0x77000 C_Cfg_Info
		0x78000 U_Cfg_Info
		0x7A000 NV_2
		0x80000 End Flash

Так, как обновление OTA не используется, то прошивка может занимать оба пространства `Firmware` и `OTA Image`, что в сумме составляет 0x62000 (401408) байт.

`bootloader` ничего не умеет, кроме, как запускать прошивку с адреса 0x8000.

В конфиге сохраняютстя только настройки модуля. Конфиг записывается в NV_2 (куда-то в область с 0x7a000 по 0x7c000). Используется модуль NV_MODULE_APP с номером NV_ITEM_APP_USER_CFG (для понимания смотрите app_cfg.h и sdk/proj/drivers/drv_nv.h)

В электросчетчиках используются разные протоколы обмена. Дополнительную информацию смотрите в разделе по конкретному [электросчетчику](#electricity_meters).

---
	
## <a id="settings">Настройка</a>

Открываем на редактирование файл `configuration.yaml` от zigbee2mqtt. И добавляем в конец файла

		external_converters:
			- electricity_meter.js
		  

Файл `electricity_meter.js` копируем из [папки проекта](https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/zigbee2mqtt/convertor/electricity_meter.js) туда же, где лежит `configuration.yaml` от zigbee2mqtt. Не забываем разрешить подключение новых устройств - `permit_join: true`. Перегружаем zigbee2mqtt. Проверяем его лог, что он запустился и нормально работает.

Первоначальная настройка происходит через web-интерфейс zigbee2mqtt. Для начала нужно убедиться, что устройство в сети. 

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/z2m_device.jpg"/>

Если во вкладке `devices` его нет, нужно нажать кнопку на устройстве 5 раз. Итак убедились, что устройство в сети, можно переходить к настройкам. Переходим к вкладке `exposes` устройства.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/z2m_exposes.jpg"/>

Видим, что в ячейке `Device model preset` выставлен счетчик `No Device`, т.е. никакой счетчик не выбран, считывание данных не происходит. Выбираем нужный счетчик из предложенных. По умолчанию период опроса счетчика - 1 минута. Если нужно больше (максимальное значение 255 минут), меняем настройку `Device measurement preset`.

---

## <a id="electricity_meters">Электросчетчики, поддерживаемые устройством</a>

### <a id="kaskad-1-mt">Однофазный многотарифный счетчик КАСКАД-1-МТ</a>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/electricity_meters/kaskad/kaskad_1_mt.jpg">

Счетчик общается по протоколу [МИРТЕК](https://github.com/slacky1965/electricity_meter_zrd/raw/version1/doc/electricity_meters/kaskad/Star_104_304_1.20.doc).

В настоящий момент устройство может прочитать из счетчика:

* 4 тарифа (в kWh)
* силу тока (в A)
* напряжение сети (в V)
* мощность (в kW)
* оставшийся ресурс батарии прибора (в %)
* полный серийный номер прибора (например 3171112520109)
* дату изготовления прибора (например 04.10.2017)

Для корректного соединения со счетчиком в разделе `exposes` в web-интерфейсе `zigbee2mqtt` нужно ввести 5 последних цифр серийного номера прибора. Например для счетчика, который изображен на фото, это 07943. Так, как первая цифра 0, то вводить нужно 7943. Также прибор этот номер показывает на дисплее, как AD-07943.

---

### <a id="kaskad-11">Однофазный многотарифный счетчик КАСКАД-11-C1</a>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/electricity_meters/kaskad/kaskad_11_c1.jpg">

Счетчик общается по своему [протоколу](https://github.com/slacky1965/electricity_meter_zrd/raw/version1/doc/electricity_meters/kaskad/2023.01.12-KASKAD_11.doc).

В настоящий момент устройство может прочитать из счетчика:

* 4 тарифа (в kWh)
* силу тока (в A)
* напряжение сети (в V)
* мощность (в kW)
* оставшийся ресурс батарии прибора (косвенно вычисляется, команды такой нет, в %)
* полный серийный номер прибора (например 3171112520109)
* дату изготовления прибора (например 04.10.2017)

Для корректного соединения со счетчиком в разделе `exposes` в web-интерфейсе `zigbee2mqtt` нужно ввести 5 последних цифр серийного номера прибора. Например для счетчика, который изображен на фото, это 32580. Также прибор этот номер показывает на дисплее, как AD-32580.

У этого счетчика нет команды запроса ресурса батареи. Поэтому оставшийся ресурс вычисляется из других параметров. Принято считать (во всяком случае разработчики этого счетчика так считают), что ресурс батареи порядка 10 лет. Следовательно, мы просто вычисляем оставшийся ресурс в месяцах между текущей датой прибора, датой производства и 120-тью месяцами (т.е. 10-тью годами). Ну а из этого потом легко получить проценты.

---

### <a id="mercury-206">Однофазный многотарифный счетчик Меркурий-206</a>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/electricity_meters/mercury/mercury-206.jpg">

Счетчик общается по [протоколу](https://www.incotexcom.ru/files/em/docs/mercury-protocol-obmena-1.pdf).

В настоящий момент устройство может прочитать из счетчика:

* 4 тарифа (в kWh)
* силу тока (в A)
* напряжение сети (в V)
* мощность (в kW)
* оставшийся ресурс батарии прибора (косвенно вычисляется, команды такой нет, в %)
* полный серийный номер прибора (например 35900555)
* дату изготовления прибора (например 10.12.2018)

Для корректного соединения со счетчиком в разделе `exposes` в web-интерфейсе `zigbee2mqtt` нужно ввести серийный номера прибора. Например для счетчика, который изображен на фото, это 36896545.

У этого счетчика нет команды запроса ресурса батареи. Поэтому оставшийся ресурс вычисляется из других параметров. Будем считать, что ресурс батареи порядка 10 лет или 120 месяцев. Также есть команда наработки счетчика в часах, отдельно от сети (при наличие входного напряжения 220 вольт) и отдельно от батареи (при отсутствии входного напряжения). Плюсуем эти значения и получаем наработку прибора в часах. А потом мы просто вычисляем оставшийся ресурс в месяцах между 120-тью месяцами (т.е. 10-тью годами) и наработкой. Ну а из этого потом легко получить проценты.

В процессе написания кода для этого счетчика выяснилось, что на первую (любую) команду между опросами, счетчик или не отвечает, или присылает какой-то мусор. А вот на последующие команды отвечает нормально. Поэтому сперва кидаем ему любую команду, делаем небольшую паузу (например 500 мсек) и только потом начинаем опрашивать счетчик.

---

### <a id="energomera-ce102m">Однофазный многотарифный счетчик Энергомера-СЕ102М</a>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/electricity_meters/energomera/102m_r5_front.jpg">

Протокол обмена на этот счетчик размещен на сайте Энергомеры, в [руководстве по эксплуатации](http://www.energomera.ru/documentations/product/ce102m_re.pdf)

В настоящий момент устройство может прочитать из счетчика:

* 4 тарифа (в kWh)
* силу тока (в A)
* напряжение сети (в V)
* мощность (в kW)
* замена батареи (до этого события ресурс будет показан, как 100%).
* полный серийный номер прибора (например 3171112520109)
* дата изготовления прибора у этого счетчика не предусмотрена.

Для этого счетчика серийный номер в web-интерфейсе `zigbee2mqtt` вводить не нужно.

---

### <a id="neva-mt124">Однофазный многотарифный счетчик Нева-МТ124</a>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/electricity_meters/neva/neva_mt124.jpg">

Протокол обмена описан в `ГОСТ Р МЭК 61107-2001`. В ГОСТе нужно читать только то, что касается режима С. Практически все тоже самое, что в протоколе обмена счетчика `Энергомера-СЕ102М`, за небольшим исключением. Вычисление контрольной суммы отличается от рекомендуемого стандартом и осуществляется как "исключающее ИЛИ". Скорость для начала обмена данными через оптопорт - 300b. После первой команды запроса на соединение, счетчик переходит на скорость 9600b.

В настоящий момент устройство может прочитать из счетчика:

* 4 тарифа (в kWh)
* мощность (в kW)
* оставшийся ресурс батарии прибора (вычисляется по напряжению батарейки, команды такой нет, в %)
* полный серийный номер прибора (например 3171112520109)
* сила тока (не поддерживается)
* напряжение сети (не поддерживается)
* дата изготовления прибора у этого счетчика не предусмотрена.

Для этого счетчика серийный номер в web-интерфейсе `zigbee2mqtt` вводить не нужно.

---

### <a id="nartis-100">Однофазный многотарифный счетчик Нартис-100</a>

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/electricity_meters/nartis/nartis100.jpg">

Счетчик общается по протоколу СПОДЭС.

В настоящий момент устройство может прочитать из счетчика:

* 4 тарифа (в kWh)
* силу тока (в A)
* напряжение сети (в V)
* мощность (в kW)
* оставшийся ресурс батарии прибора (косвенно вычисляется, команды такой нет, в %)
* полный серийный номер прибора (например 21021839)
* дату изготовления прибора (например 21.10.2021)

У этого счетчика нет команды запроса ресурса батареи. Поэтому оставшийся ресурс вычисляется между текущей датой прибора, датой производства.

---

Если счетчик выбран и устройство примагничено к окошку оптопорта счетчика, то в `exposes` web-интерфейса `zigbee2mqtt` получим примерно такую картинку.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/z2m_nartis100.jpg">

Если данные не считываются, есть несколько причин.

* Выбран не тот счетчик.
* Не введен адрес счетчика, где он требуется.
* Не совмещены светодиод и фототранзистор на устройстве и счетчике. Рекомендуется немного подвигать устройство.
	
---

## <a id="home_assistant">Home Assistant</a>

В результате вы получите примерно вот такую картинку.

<img src="https://raw.githubusercontent.com/slacky1965/electricity_meter_zrd/version1/doc/images/ha.jpg">

