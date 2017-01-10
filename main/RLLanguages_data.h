#pragma pack(push, 1)

struct MessageStr {
	UINT16 key;
	CHAR* str;
};

// LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US
// code_page(1252)
struct MessageStr StringsEnglish[] = {
	{ D_CLIENT,		            "Client" },
	{ D_OPERATOR,	            "Operator" },
	{ D_CONNECT,                "Connect" },
	{ D_EXIT,					"Exit" },
	{ D_SERVICE,				"Service" },
	{ D_INSTALL,				"Install" },
	{ D_START,					"Start" },
	{ D_STOP,					"Stop" },
	{ D_REMOVE,					"Remove" },
	{ D_ADD,					"Add" },
	{ D_EDIT,					"Edit" },
	{ D_SETTINGS,				"Settings" },
	{ D_SETTINGS_COMMON,		"Common" },
	{ D_SETTINGS_NETWORK,		"Network" },
	{ D_ACCEPT,                 "&Accept" },
	{ D_REJECT,                 "&Reject" },
	{ D_YOUR_ID,                "Your ID" },
	{ D_CLIENT_ID,              "Client ID/ IP" },
	{ D_VIEW_ONLY,				"View only" },
	{ D_CLIENT_ID_NOT_PROVIDED, "Please enter client ID/ IP" },
	{ D_ACCESS_PERMISSIONS,     "Access Permissions" },
	{ D_CONTACT_BOOK,	        "Contact book" },
	{ D_REQUEST_ROUTER,			"Request client's router IP" },
	{ D_WARN_FULL_SCREEN,		"Warn at switching to the full-screen mode" },
	{ D_LOGGING_FOR_DEBUG,		"Log debugging info into file" },
	{ D_RUN_UNDER_SYSTEM,		"Run under SYSTEM account on %S" },
	{ D_PROTECT_SETTINGS,		"Protect these settings from remote operator" },
	{ D_OFF_DESKTOP_BACKGROUND,	"Disable desktop background" },
	{ D_OFF_DESKTOP_COMPOSITION,"Disable desktop composition" },
	{ D_OFF_DESKTOP_EFFECTS,    "Disable visual effects" },
	{ D_SHOW_HINTS,				"Show tooltip - mouse cursor blinking cause" },
	{ D_PCNotFound,             "The computer ID=%u wasn't found." },
	{ D_REMOTEPC_CLOSE,         "The remote computer closed this session." },
	{ D_WARN_DIFF_VERSIONS,     "The remote pc uses Ammyy Admin v%s\nIt's other version, so the program can work incorrectly!" },
	{ D_ACCEPT_CONN_TEXT,       "Operator%s is connecting to your PC." }, // %s => " (ID=1234)" or ""
	{ D_REMEMBER_MY_ANSWER,     "Remember my answer for this operator" },
	//{ D_ENABLE_INPUT,           "Enable remote keyboard and mouse" },
	{ D_VIEW_SCREEN,            "View screen" },
	{ D_REMOTE_CONTROL,			"Remote control" },
	{ D_ENABLE_FS,              "File manager" },
	{ D_AUDIO_CHAT,				"Audio chat" },
	{ D_RDP_SESSION,			"RDP sessions" },	
	{ D_ACCESSREJECTED,         "Remote computer rejected your query to access." },
	{ D_WAITING_AUTHORIZATION,  "Waiting for authorization from remote PC" },
	{ D_SESSION_INACTIVE,       "This session is inactive.\nYou can reconnect to an active session in a few seconds." },
	{ D_LOCAL_CURSOR_SHAPES,	"Dot|Small dot|Normal arrow|No cursor" },
	{ D_REMOTE_CURSOR_SHAPES,	"Don't show|Show shape|Show shape and position" },
	{ D_ACCESS_FILES_CUR_USER,	"Access files under current user account" },
	{ D_START_CLIENT,           "Start \"wait for session\" mode automatically" },
	{ D_WAIT_FOR_SESSION,		"Wait for session" },
	{ D_CREATE_SESSION,			"Create session" },
	{ D_COMPUTER_BUSY,          "The remote computer exceeded concurrent sessions limit" },
	{ D_EXCEED_SESSIONS_VIEWER, "You've exceeded %u sessions limit for %S license on this computer" },
	{ D_BUY_LICENSE,            "Please consider buying a license" }
};

// LANGUAGE LANG_RUSSIAN, SUBLANG_DEFAULT
// code_page(1251)
struct MessageStr StringsRussian[] = {
	{ D_START,	                "Запустить" },
	{ D_STOP,                   "Остановить" },
	{ D_CONNECT,                "Подключиться" },
	{ D_YOUR_ID,                "Ваш ID" },
	{ D_CLIENT,		            "Клиент" },
	{ D_OPERATOR,	            "Оператор" },
	{ D_CLIENT_ID_NOT_PROVIDED, "Пожалуйста, укажите ID/ IP клиента" },
	{ D_CLIENT_ID,              "ID/ IP клиента" },
	{ D_VIEW_ONLY,				"Только просмотр" },
	{ D_PCNotFound,             "Компьютер ID=%u не найден" },
	{ D_REMOTEPC_CLOSE,         "Удаленный компьютер завершил текущую сессию." },
	{ D_WARN_DIFF_VERSIONS,     "Удаленные компьютер использует Ammyy Admin v%s\n Это другая версия, поэтому программа может работать некорректно!" },
	{ D_ACCEPT,                 "&Разрешить" },
	{ D_REJECT,                 "&Отклонить" },
	{ D_ACCEPT_CONN_TEXT,       "Оператор%s хочет установить с Вами соединение." },
	{ D_REMEMBER_MY_ANSWER,     "Запомнить мой ответ для данного оператора" },
	//{ D_ENABLE_INPUT,           "Разрешить управление клавиатурой и мышью с удаленного компьютера" },
	{ D_VIEW_SCREEN,			"Просмотр экрана" },
	{ D_REMOTE_CONTROL,			"Удаленное управление" },
	{ D_ENABLE_FS,              "Менеджер Файлов" },
	{ D_AUDIO_CHAT,				"Голосовой чат" },
	{ D_RDP_SESSION,			"RDP сессии" },
	{ D_ACCESSREJECTED,         "Удаленный компьютер отклонил ваш запрос на подключение." },
	{ D_WAITING_AUTHORIZATION,  "Ожидание авторизации от удаленного компьютера." },
	{ D_ACCESS_PERMISSIONS,     "Права Доступа" },
	{ D_CONTACT_BOOK,	        "Контактная книга" },
	{ D_EXIT,                   "Выйти" },
	{ D_SERVICE,                "Служба" },
	{ D_INSTALL,                "Установить" },
	{ D_REMOVE,                 "Удалить" },
	{ D_ADD,                    "Добавить" },
	{ D_EDIT,                   "Редактировать" },
	{ D_SETTINGS,               "Настройки" },
	{ D_SETTINGS_COMMON,        "Общие" },
	{ D_SETTINGS_NETWORK,       "Сеть" },
	{ D_REQUEST_ROUTER,			"Запрашивать IP клиентского роутера" },
	{ D_WARN_FULL_SCREEN,		"Предупреждать при переключении на полноэкранный режим" },
	{ D_LOGGING_FOR_DEBUG,		"Записывать в файл отладочную информацию" },
	{ D_RUN_UNDER_SYSTEM,		"Запускать под системным аккаунтом на %S" },
	{ D_PROTECT_SETTINGS,		"Защитить эти настройки от удаленного оператора" },
	{ D_OFF_DESKTOP_BACKGROUND,	"Отключать фоновый рисунок рабочего стола" },
	{ D_OFF_DESKTOP_EFFECTS,    "Отключать визуальные эффекты" },
	{ D_OFF_DESKTOP_COMPOSITION,"Отключать композицию рабочего стола" },
	{ D_SHOW_HINTS,				"Показывать всплывающие подсказки - причина мерцания курсора мышки" },
	{ D_ACCESS_FILES_CUR_USER,	"Доступ к файлам под аккаунтом текущего пользователя" },
	{ D_START_CLIENT,           "Автоматически запускать режим ожидания сеанса" },
	{ D_WAIT_FOR_SESSION,		"Ожидание сеанса" },
	{ D_CREATE_SESSION,			"Создание сеанса" },
	{ D_COMPUTER_BUSY,          "Удаленный компьютер превысил лимит одновременных сессий" },
	{ D_EXCEED_SESSIONS_VIEWER, "Вы превысили лимит %u сессии для %S лицензии на этом компьютере" },
	{ D_BUY_LICENSE,            "Пожалуйста, рассмотрите возможность покупки лицензии" }
};

// LANGUAGE LANG_HUNGARIAN, SUBLANG_DEFAULT
// code_page(1250)
struct MessageStr StringsHungarian[] = {
    { D_START,					"Indнtбs" },
    { D_CONNECT,				"Kapcsolуdбs" },
    { D_STOP,					"Leбllнtбs" },
    { D_YOUR_ID,				"Az Цn ID-je" },
	{ D_CLIENT,					"Kliens" },
	{ D_OPERATOR,				"operбtor" },
    { D_CLIENT_ID_NOT_PROVIDED, "Kйrem, adja meg a kliens ID-jйt" },
    { D_CLIENT_ID,				"Kliens ID/IP" },
    { D_PCNotFound,				"A kцvetkezo ID nem talбlhatу: %u" },
    { D_REMOTEPC_CLOSE,			"A tбvoli szбmнtуgйp megszakнtotta a kapcsolatot" },
    { D_WARN_DIFF_VERSIONS,		"A tбvoli szбmнtуgйp бltal hasznбlt Ammyy Admin verziу: v%s\nMivel a verziу eltйro, hibбs mukцdйs lйphet fel." },
    { D_ACCEPT,					"&Elfogad" },
    { D_REJECT,					"&Elutasнt" },
    { D_ACCEPT_CONN_TEXT,		"A kцvetkezo%s szбmъ operбtor az Цn szбmнtуgйpйhez kapcsolуdik" },
    { D_REMEMBER_MY_ANSWER,		"Emlйkezzen a vбlaszomra ezt az operбtort illetoen" },
    //{ D_ENABLE_INPUT,			"Tбvoli egйr йs billentyuzet engedйlyezйse" },
    { D_ENABLE_FS,				"Fбjlrendszer elйrйsйnek engedйlyezйse" },
    { D_ACCESSREJECTED,			"A tбvoli szбmнtуgйp visszautasнtotta a kapcsolat elfogadбsбt." },
    { D_WAITING_AUTHORIZATION,	"Vбrakozбs a tбvoli szбmнtуgйp engedйlyйre." },
    { D_SESSION_INACTIVE,		"A kapcsolat inaktнv.\nЪjrakapcsolуdhat nйhбny mбsodperc mъlva." },
    { D_COMPUTER_BUSY,			"A tбvoli szбmнtуgйpen egy mбsik kapcsolat mбr aktнv." }
};

// LANGUAGE LANG_DANISH, SUBLANG_DEFAULT
// code_page(1252)
struct MessageStr StringsDanish[] = {
    { D_START,					"Start" },
    { D_CONNECT,				"Tilslut" },
    { D_STOP,					"Stop" },
    { D_YOUR_ID,				"ID" },
	{ D_CLIENT,					"Klient" },
	{ D_OPERATOR,				"Operatшr" },
    { D_CLIENT_ID_NOT_PROVIDED, "Venligst angiv Klient-ID" },
    { D_CLIENT_ID,				"Klient-ID/IP" },
    { D_PCNotFound,				"Computer med ID=%u ikke fundet" },
    { D_REMOTEPC_CLOSE,			"Fjernbrugeren afsluttede sessionen" },
    { D_WARN_DIFF_VERSIONS,		"Fjernbrugeren anvender Ammyy Admin v%s\n Programmet virker muligvis ikke korekt." },
    { D_ACCEPT,					"&Godkend" },
    { D_REJECT,					"&Afvis" },
    { D_ACCEPT_CONN_TEXT,		"Admin med%s tilslutter til din PC" },
    { D_REMEMBER_MY_ANSWER,		"Gem mit svar" },
    { D_ENABLE_INPUT,			"Tillad fjernstyring af mus og tastatur." },
    { D_ENABLE_FS,				"Tillad adgang til filer" },
    { D_ACCESSREJECTED,			"Adgang nжgtet" },
    { D_WAITING_AUTHORIZATION,	"Afventer godkendelse fra fjern PC." },
    { D_SESSION_INACTIVE,		"Denne session er inaktiv. \n Du kan tilslutte igen om et шjeblik." },
    { D_COMPUTER_BUSY,			"Fjerncomputeren er optaget af en anden session." }
};

// LANGUAGE LANG_GERMAN, SUBLANG_GERMAN
// code_page(1252)
struct MessageStr StringsGerman[] = {
    { D_START,					"Start" },
    { D_CONNECT,				"Verbinden" },
    { D_STOP,					"Stop" },
    { D_YOUR_ID,				"Ihre ID" },
	{ D_CLIENT,					"Kunde" },
	{ D_OPERATOR,				"Operator" },
    { D_CLIENT_ID,				"Kunden ID/IP" },
    { D_PCNotFound,				"Der Computer mit der ID=%u wurde nicht gefunden." },
    { D_REMOTEPC_CLOSE,			"Der entfernte Computer hat die Verbindung beendet." },
    { D_WARN_DIFF_VERSIONS,		"Das Programm kann nicht korrekt arbeiten, da der entfernte Computer eine andere Version von Ammyy Admin v%s\n verwendet." },
    { D_ACCEPT,					"&Akzeptieren" },
    { D_REJECT,					"&Verwerfen" },
    { D_ACCEPT_CONN_TEXT,		"Operator mit%s verbindet sich mit Ihrem Computer." },
    { D_REMEMBER_MY_ANSWER,		"Antwort fьr diesen Operator merken" },
    { D_ENABLE_INPUT,			"Aktiviere Fernsteuerung der Tastatur und der Maus" },
    { D_ENABLE_FS,				"Aktiviere Zugriff auf das Dateisystem" },
    { D_ACCESSREJECTED,			"Der entfernter Computer lehnt Ihre Zugriffsanfrage ab." },
    { D_WAITING_AUTHORIZATION,	"Warten auf Autorisierung durch den entfernten Computer" },
    { D_SESSION_INACTIVE,		"Die Session ist inaktiv.\n Sie kцnnen sich mit der aktive Session in wenigen Sekunden erneut verbinden." },
    { D_COMPUTER_BUSY,			"Der entfernte Computer ist durch eine andere Session belegt." }
};

// LANGUAGE LANG_FINNISH, SUBLANG_DEFAULT
// code_page(1252)
struct MessageStr StringsFinnish[] = {
    { D_START,					"Aloita" },
    { D_CONNECT,				"Yhdistд" },
    { D_STOP,					"Pysдytд" },
    { D_YOUR_ID,				"ID-numero" },
	{ D_CLIENT,					"Asiakas" },
	{ D_OPERATOR,				"Operaattori" },
    { D_CLIENT_ID_NOT_PROVIDED, "Syцtд asiakas-ID" },
    { D_CLIENT_ID,				"Asiakas-ID/IP" },
    { D_PCNotFound,				"Tietokonetta ID:llд %u ei lцydy." },
    { D_REMOTEPC_CLOSE,			"Etдtietokone sulki yhteyden." },
    { D_WARN_DIFF_VERSIONS,		"Ohjelma ei voi toimia oikein. Etдtietokoneen kдytцssд on eri versio Ammyy Admin v%s\n ohjelmistosta." },
    { D_ACCEPT,					"&Hyvдksy" },
    { D_REJECT,					"H&ylkдд" },
    { D_ACCEPT_CONN_TEXT,		"Operaattori%s yrittдд yhdistдд tietokoneellesi." },
    { D_REMEMBER_MY_ANSWER,		"Muista valinta tдlle operaattorille" },
    { D_ENABLE_INPUT,			"Salli nдppдimistцn ja hiiren kдyttц" },
    { D_ENABLE_FS,				"Salli tiedostojen kдyttц" },
    { D_ACCESSREJECTED,			"Etдtietokone hylkдsi yhteysyrityksen." },
    { D_WAITING_AUTHORIZATION,	"Odotetaan valtuutusta etдtietokoneelta" },
    { D_SESSION_INACTIVE,		"Istunto ei ole aktiivinen. \nAktivoidaksesi istunnon, voit yhdistдд uudelleen hetken kuluttua." },
    { D_COMPUTER_BUSY,			"Varattu, etдtietokoneessa on toinen istunto auki." }
};

// LANGUAGE LANG_FRENCH, SUBLANG_FRENCH
// code_page(1252)
struct MessageStr StringsFrench[] = {
    { D_START,					"Dйmarrer" },
    { D_CONNECT,				"Connexion" },
    { D_STOP,					"Arrкter" },
    { D_YOUR_ID,				"Votre ID" },
	{ D_CLIENT,					"Client" },
	{ D_OPERATOR,				"Opйrateur" },
    { D_CLIENT_ID_NOT_PROVIDED, "Veuillez saisir l'ID client" },
    { D_CLIENT_ID,				"ID/IP Client" },
    { D_PCNotFound,				"Impossible de trouver l'ordinateur distant (ID=%u)." },
    { D_REMOTEPC_CLOSE,			"L'ordinateur distant a fermй la session." },
    { D_WARN_DIFF_VERSIONS,		"L'ordinateur distant utilise Ammy Admin v%s\nC'est une autre version et la connexion est impossible." },
    { D_ACCEPT,					"&Accepter" },
    { D_REJECT,					"&Rejeter" },
    { D_ACCEPT_CONN_TEXT,		"L'opйrateur%s se connecte votre ordinateur." },
    { D_REMEMBER_MY_ANSWER,		"Mйmoriser ma rйponse pour cet opйrateur" },
    { D_ENABLE_INPUT,			"Activer le clavier et la souris distants" },
    { D_ENABLE_FS,				"Activer l'accиs aux fichiers" },
    { D_ACCESSREJECTED,			"L'ordinateur distant a rejetй votre demande." },
    { D_WAITING_AUTHORIZATION,	"En attente d'autorisation..." },
    { D_SESSION_INACTIVE,		"Cette session est inactive.\nVous pourrez vous reconnecter dans quelques secondes." },
    { D_COMPUTER_BUSY,			"L'ordinateur distant est dйjа connectй sur une autre session." }
};

// LANGUAGE LANG_ITALIAN, SUBLANG_ITALIAN
// code_page(1252)
struct MessageStr StringsItalian[] = {
    { D_START,					"Avvia" },
    { D_CONNECT,				"Connetti" },
    { D_STOP,					"Arresta" },
    { D_YOUR_ID,				"Il tuo ID" },
	{ D_CLIENT,					"Utente" },
	{ D_OPERATOR,				"Operatore" },
    { D_CLIENT_ID_NOT_PROVIDED, "Inserisci ID" },
    { D_CLIENT_ID,				"ID/IP Client" },
    { D_PCNotFound,				"Impossibile trovare il computer con ID=%u." },
    { D_REMOTEPC_CLOSE,			"Il computer remoto ha chiuso la sessione." },
    { D_WARN_DIFF_VERSIONS,		"Il computer remoto utilizza Ammyy Admin v%s\nIl programma non pu? funzionare correttamente." },
    { D_ACCEPT,					"&Accetta" },
    { D_REJECT,					"&Rifiuta" },
    { D_ACCEPT_CONN_TEXT,		"Un'operatore con%s si sta connettendo al tuo computer." },
    { D_REMEMBER_MY_ANSWER,		"Ricorda la mia risposta per questo operatore" },
    { D_ENABLE_INPUT,			"Abilita mouse e tastiera remoti" },
    { D_ENABLE_FS,				"Abilita l'accesso al file system" },
    { D_ACCESSREJECTED,			"Il computer remoto ha rifiutato la tua richiesta di accesso." },
    { D_WAITING_AUTHORIZATION,	"In attesa dell'Autorizzazione dal computer remoto" },
    { D_SESSION_INACTIVE,		"Questa sessione non ? attiva.\nPuoi riconnetterti alla sessione attiva tra qualche secondo." },
    { D_COMPUTER_BUSY,			"Il computer remoto ? occupato in un'altra sessione." }
};

// LANGUAGE LANG_DUTCH, SUBLANG_DUTCH
// code_page(1252)
struct MessageStr StringsDutch[] = {
    { D_START,					"Start" },
    { D_CONNECT,				"Verbinden" },
    { D_STOP,					"Stop" },
    { D_YOUR_ID,				"Uw ID" },
	{ D_CLIENT,					"Client" },
	{ D_OPERATOR,				"Operator" },
    { D_CLIENT_ID_NOT_PROVIDED, "Vul client ID in" },
    { D_CLIENT_ID,				"Client ID/IP" },
    { D_PCNotFound,				"De computer ID=%u is niet gevonden." },
    { D_REMOTEPC_CLOSE,			"De andere computer heeft de sessie afgebroken." },
    { D_WARN_DIFF_VERSIONS,		"De andere computer gebruikt Ammyy Admin v%s\nl's andere versie, en kan problemen geven!" },
    { D_ACCEPT,					"&Accepteer" },
    { D_REJECT,					"&Weiger" },
    { D_ACCEPT_CONN_TEXT,		"Operator met%s is verbinding aan het maken met uw PC." },
    { D_REMEMBER_MY_ANSWER,		"Onthoudt mijn antwoord voor deze operator" },
    { D_ENABLE_INPUT,			"Activeer toetsenbord en muis van andere computer" },
    { D_ENABLE_FS,				"Activeer toegang tot bestanden" },
    { D_ACCESSREJECTED,			"De andere computer heeft toegang geweigerd." },
    { D_WAITING_AUTHORIZATION,	"Op toestemming wachten van andere PC" },
    { D_SESSION_INACTIVE,		"De sessie is niet actief.\n U kunt opnieuw verbinden in enkele seconden" },
    { D_COMPUTER_BUSY,			"De andere computer is bezig met een andere sessie." }
};

// LANGUAGE LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL
// code_page(1252)
struct MessageStr StringsNorwegian[] = {
    { D_START,					"Start" },
    { D_CONNECT,				"Tilkople" },
    { D_STOP,					"Avbryt" },
    { D_YOUR_ID,				"Dinn ID" },
	{ D_CLIENT,					"Klient" },
	{ D_OPERATOR,				"Operatшr" },
    { D_CLIENT_ID_NOT_PROVIDED, "Vennligst oppgi klientens ID" },
    { D_CLIENT_ID,				"Klient-ID" },
    { D_PCNotFound,				"Klienten med ID = %U ble ikke funnet" },
    { D_REMOTEPC_CLOSE,			"Klientmaskinen avsluttet sesjonen" },
    { D_WARN_DIFF_VERSIONS,		"Klientmaskinen bruker Ammyy Admin v%s\nDet er en annen versjon, og feil kan oppstе" },
    { D_ACCEPT,					"&Godta" },
    { D_REJECT,					"&Avslе" },
    { D_ACCEPT_CONN_TEXT,		"Operatшr med%s kopler til din maskin" },
    { D_REMEMBER_MY_ANSWER,		"Husk mine innstillinger for denne operatшren" },
    { D_ENABLE_INPUT,			"Tillat fjernstyring av mus og tastatur" },
    { D_ENABLE_FS,				"Tillat tilgang til filsystemet" },
    { D_ACCESSREJECTED,			"Klienten nektet deg tilgang" },
    { D_WAITING_AUTHORIZATION,	"Venter pе tillatelse fra klienten" },
    { D_SESSION_INACTIVE,		"Denne sesjonen er inaktiv.\n Du kan kople til pе nytt om noen sekunder" },
    { D_COMPUTER_BUSY,			"Klienten er opptatt med en annen sesjon" }
};

// LANGUAGE LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN
// code_page(1252)
struct MessageStr StringsPortuguese[] = {
    { D_START,					"Comeзar" },
    { D_CONNECT,				"Conectar" },
    { D_STOP,					"Parar" },
    { D_YOUR_ID,				"Sua ID" },
	{ D_CLIENT,					"Cliente" },
	{ D_OPERATOR,				"Operador" },
    { D_CLIENT_ID_NOT_PROVIDED, "Por favor insira a ID do Cliente" },
    { D_CLIENT_ID,				"Cliente ID/IP" },
    { D_PCNotFound,				"O Computador ID=%u nгo foi encontrado." },
    { D_REMOTEPC_CLOSE,			"O Computador Remoto fechou essa sessгo." },
    { D_WARN_DIFF_VERSIONS,		"O Computador Remoto usa outra versгo do Ammyy Admin v%s\nЙ outra versгo, o programa poderб trabalhar incorretamente!" },
    { D_ACCEPT,					"&Aceitar" },
    { D_REJECT,					"&Rejeitar" },
    { D_ACCEPT_CONN_TEXT,		"O Operador com%s esta conectando ao seu PC." },
    { D_REMEMBER_MY_ANSWER,		"Relembrar minha resposta para este operador" },
    { D_ENABLE_INPUT,			"Permitir teclado e mouse remoto" },
    { D_ENABLE_FS,				"Permitir acesso ao arquivo de sistema" },
    { D_ACCESSREJECTED,			"O computador Remoto rejeitou seu pedido de acesso." },
    { D_WAITING_AUTHORIZATION,	"Esperando Autorizaзгo do PC Remoto." },
    { D_SESSION_INACTIVE,		"Esta sessгo esta inativa.\nVocк poderб reconectar em alguns segundos." },
    { D_COMPUTER_BUSY,			"O computador Remoto estб ocupado com outra sessгo." },
};

// LANGUAGE LANG_SWEDISH, SUBLANG_DEFAULT
// code_page(1252)
struct MessageStr StringsSwedish[] = {
    { D_START,					"Start" },
    { D_CONNECT,				"Anslut" },
    { D_STOP,					"Stopp" },
    { D_YOUR_ID,				"Ditt ID" },
	{ D_CLIENT,					"Klient" },
	{ D_OPERATOR,				"Operatцr" },
    { D_CLIENT_ID_NOT_PROVIDED, "Vдnligen ange klientens ID" },
    { D_CLIENT_ID,				"Klient-ID/IP" },
    { D_PCNotFound,				"Datorn med ID=%u hittades inte" },
    { D_REMOTEPC_CLOSE,			"Fjдrrdatorn avslutade sessionen" },
    { D_WARN_DIFF_VERSIONS,		"Fjдrrdatorn anvдnder Ammyy Admin v%s\nDet дr en annan version, sе programmet kanske inte fungerar." },
    { D_ACCEPT,					"&Acceptera" },
    { D_REJECT,					"&Neka" },
    { D_ACCEPT_CONN_TEXT,		"Operatцr med%s ansluter till din PC." },
    { D_REMEMBER_MY_ANSWER,		"Kom ihеg mitt svar fцr denna operatцr" },
    { D_ENABLE_INPUT,			"Tillеt fjдrrstyrning av tangentbord och mus" },
    { D_ENABLE_FS,				"Tillеt еtkomst till filsystemet" },
    { D_ACCESSREJECTED,			"Fjдrrdatorn nekade еtkomst." },
    { D_WAITING_AUTHORIZATION,	"Vдntar pе tillеtelse frеn fjдrdatorn" },
    { D_SESSION_INACTIVE,		"Denna session дr inaktiv.\n Du kan еteransluta till aktiv session om nеgra sekunder." },
    { D_COMPUTER_BUSY,			"Fjдrrdatorn дr upptagen med andra sessioner." }
};

// LANGUAGE LANG_SPANISH, SUBLANG_SPANISH_MODERN
// code_page(1252)
struct MessageStr StringsSpanish[] = {
    { D_START,					"Comenzar" },
    { D_CONNECT,				"Conectar" },
    { D_STOP,					"Detener" },
    { D_YOUR_ID,				"Su ID" },
	{ D_CLIENT,					"Cliente" },
	{ D_OPERATOR,				"Operador" },
    { D_CLIENT_ID_NOT_PROVIDED, "Por favor ingrese el ID del cliente" },
    { D_CLIENT_ID,				"ID/IP Cliente" },
    { D_PCNotFound,				"El ordenador con el ID=%u no se ha encontrado." },
    { D_REMOTEPC_CLOSE,			"El ordenador remoto ha finalizado la sesiуn." },
    { D_WARN_DIFF_VERSIONS,		"La PC remota utiliza otra versiуn de Ammyy Admin v%s\nIt's, por lo que la aplicaciуn podrнa no funcionar correctamente." },
    { D_ACCEPT,					"&Aceptar" },
    { D_REJECT,					"&Rechazar" },
    { D_ACCEPT_CONN_TEXT,		"El Operador con la%s estб conectandose a su PC." },
    { D_REMEMBER_MY_ANSWER,		"Recordar la respuesta para йste operador." },
    { D_ENABLE_INPUT,			"Habilitar mouse y teclado remotos." },
    { D_ENABLE_FS,				"Habilitar el acceso al sistema de archivos." },
    { D_ACCESSREJECTED,			"El ordenador remoto rechazу su intento de acceso." },
    { D_WAITING_AUTHORIZATION,	"Esperando Autorizaciуn de la PC remota." },
    { D_SESSION_INACTIVE,		"La sesiуn se encuentra inactiva.\nPuede reconectarse a la sesiуn activa en unos segundos." },
    { D_COMPUTER_BUSY,			"El ordenador remoto estб ocupado con otra sesiуn." }
};

// LANGUAGE LANG_GREEK, SUBLANG_DEFAULT
// code_page(1253)
struct MessageStr StringsGreek[] = {
    { D_START,					"ёнбсоз" },
    { D_CONNECT,				"Уэндеуз" },
    { D_STOP,					"ЛЮоз" },
    { D_YOUR_ID,				"Фп ID убт" },
	{ D_CLIENT,					"РелЬфзт" },
	{ D_OPERATOR,				"ЧейсйуфЮт" },
    { D_CLIENT_ID_NOT_PROVIDED, "Рбсбкблю ейуЬгефе фп ID фпх релЬфз" },
    { D_CLIENT_ID,				"ID/IP РелЬфз" },
    { D_PCNotFound,				"П хрплпгйуфЮт ID=%u ден всЭизке" },
    { D_REMOTEPC_CLOSE,			"П брпмбксхумЭнпт хрплпгйуфЮт дйЭкпше фзн уэндеуз" },
    { D_WARN_DIFF_VERSIONS,		"П брпмбксхумЭнпт хрплпгйуфЮт Эчей екдпуз v%s\nIt's фпх Ammyy Admin. Мрпсей нб мзн хрбсчей псиз лейфпхсгЯб." },
    { D_ACCEPT,					"&БрпдпчЮ" },
    { D_REJECT,					"Б&рьсйшшз" },
    { D_ACCEPT_CONN_TEXT,		"П чейсйуфЮт ме%s ухндЭефбй уфпн хрплпгйуфЮ убт." },
    { D_REMEMBER_MY_ANSWER,		"Брпмнзмьнехуз брЬнфзузт гйб бхфьн фпн чейсйуфЮ" },
    { D_ENABLE_INPUT,			"ЕнесгпрпЯзуз брпмбксхумЭнпх рлзкфсплпгЯпх кбй рпнфйкйпэ" },
    { D_ENABLE_FS,				"ЕнесгпрпЯзуз рсьувбузт уфб бсчеЯб" },
    { D_ACCESSREJECTED,			"П брпмбксхумЭнпт хрплпгйуфЮт брЭссйше фп бЯфзмб" },
    { D_WAITING_AUTHORIZATION,	"БнбмпнЮ Эггсйузт брь фпн брпмбксхумЭнп хрплпгйуфЮ" },
    { D_SESSION_INACTIVE,		"З уэндеуз еЯнбй бненесгЮ.\n МрпсеЯфе нб обнбухндеиеЯфе уе лЯгб дехфесьлерфб." },
    { D_COMPUTER_BUSY,			"П брпмбксхумЭнпт хрплпгйуфЮт еЯнбй уе Ьллз енесгЮ ухндеуз." }
};

// LANGUAGE LANG_TURKISH, SUBLANG_DEFAULT
// code_page(1254)
struct MessageStr StringsTurkish[] = {
    { D_START,					"Baюlat" },
    { D_CONNECT,				"Baрlan" },
    { D_STOP,					"Durdur" },
    { D_YOUR_ID,				"ID'niz" },
	{ D_CLIENT,					"Эstemci" },
	{ D_OPERATOR,				"Operatцr" },
    { D_CLIENT_ID_NOT_PROVIDED, "Lьtfen istemci ID'sini girin" },
    { D_CLIENT_ID,				"Эstemci ID'si" },
    { D_PCNotFound,				"%u ID'li bilgisayar bulunamadэ." },
    { D_REMOTEPC_CLOSE,			"Uzak bilgisayar oturumu kapattэ." },
    { D_WARN_DIFF_VERSIONS,		"Uzak bilgisayar Ammyy Admin'in %s sьrьmьnь kullanэyor.\nSьrьmler farklэ, bu nedenle yazэlэm dьzgьn зalэюmayabilir!" },
    { D_ACCEPT,					"&Kabul Et" },
    { D_REJECT,					"&Reddet" },
    { D_ACCEPT_CONN_TEXT,		"%s operatцr, bilgisayarэnэza baрlanэyor." },
    { D_REMEMBER_MY_ANSWER,		"Bu operatцr iзin cevabэmэ hatэrla" },
    { D_ENABLE_INPUT,			"Uzaktan klavye ve fare yцnetimini etkinleюtir" },
    { D_ENABLE_FS,				"Dosya sistemine eriюimi etkinleюtir" },
    { D_ACCESSREJECTED,			"Uzak bilgisayar eriюim isteрinizi reddetti." },
    { D_WAITING_AUTHORIZATION,	"Uzak bilgisayardan izin bekleniyor" },
    { D_SESSION_INACTIVE,		"Bu oturum etkin deрil.\nEtkin oturuma birkaз saniye iзinde tekrar baрlanabilirsiniz." },
    { D_COMPUTER_BUSY,			"Uzak bilgisayar baюka bir oturum ile meюgul." }
};

struct MessageStr StringsPolish[] = {
	{ D_CLIENT,		            "Klient" },
	{ D_OPERATOR,	            "Operator" },
	{ D_CONNECT,                "Poі№cz" },
	{ D_EXIT,					"Wyjњcie" },
	{ D_SERVICE,				"Usіuga" },
	{ D_INSTALL,				"Zainstaluj" },
	{ D_START,					"Start" },
	{ D_STOP,					"Stop" },
	{ D_REMOVE,					"Usuс" },
	{ D_ADD,					"Dodaj" },
	{ D_EDIT,					"Edytuj" },
	{ D_SETTINGS,				"Ustawienia" },
	{ D_SETTINGS_COMMON,		"Ustawienia wspуlne" },
	{ D_SETTINGS_NETWORK,		"Sieж" },
	{ D_ACCEPT,                 "&Akceptuj" },
	{ D_REJECT,                 "&Odzrzuж" },
	{ D_YOUR_ID,                "Twoje ID" },
	{ D_CLIENT_ID,              "ID/IP Klienta" },
	{ D_CLIENT_ID_NOT_PROVIDED, "Proszк podaж ID klienta" },
	{ D_ACCESS_PERMISSIONS,     "Ustawienia dostкpu" },
	{ D_CONTACT_BOOK,	        "Ksi№їka adresowa" },
	{ D_REQUEST_ROUTER,			"Zapytanie o IP routera klienta" },
	{ D_WARN_FULL_SCREEN,		"Uwaga przeі№czenie w tryb peіnego ekranu" },
	{ D_LOGGING_FOR_DEBUG,		"Logowanie debugowania do pliku" },
	{ D_RUN_UNDER_SYSTEM,		"Dziaіanie na koncie klienta %S" },
	{ D_PROTECT_SETTINGS,		"Chroс przed zmian№ ze zdalnej lokalizacji" },
	{ D_OFF_DESKTOP_BACKGROUND,	"Zablokuj zmiany tapety pulpitu" },
	{ D_OFF_DESKTOP_COMPOSITION,"Zablokuj zmiany kompozycji pulpitu" },
	{ D_OFF_DESKTOP_EFFECTS,    "Zablokuj efekty wizulane" },
	{ D_SHOW_HINTS,				"Pokazuj podpowiedzi" },
	{ D_PCNotFound,             "ID=%u Komputera nie zostaіo znalezione" },
	{ D_REMOTEPC_CLOSE,         "Zdalny komputer przerwaі poі№czenie" },
	{ D_WARN_DIFF_VERSIONS,     "Zdalny komputer uїywa innej wersji Ammyy Admin. v%s\n Moїe to powodowaж nieprawidіowoњci w dziaіaniu" },
	{ D_ACCEPT_CONN_TEXT,       "Operator o%s prуbuje nawi№zaж z Tob№ poі№czenie" },
	{ D_REMEMBER_MY_ANSWER,     "Pamiкtaj moj№ odpowiedџ dla tego operatora" },
	{ D_ENABLE_INPUT,           "Zezwуl na korzystanie ze zdealnej klawiatury i myszki" },
	{ D_ENABLE_FS,              "Zezwуl na korzystanie z managera plikуw" },
	{ D_ACCESSREJECTED,         "Zdalny komputer odmуwiі dostкpu" },
	{ D_WAITING_AUTHORIZATION,  "Oczekiwanie na autoryzacje ze zdalnego komputera" },
	{ D_SESSION_INACTIVE,       "Ta sesja jest nieaktywna.\nMoїesz siк poі№czyж ponownie za kilka sekund" },
	{ D_COMPUTER_BUSY,          "Zdalny komputer obsіuguje inn№ sesje" },
	{ D_ACCESS_FILES_CUR_USER,	"Dostкp do plikуw z bieї№cego konta uїytkownika" },
};


#define TABLE(x) x, sizeof(x)/sizeof(x[0])	
	
struct tagLanguages {
	UINT16 codepage;
	MessageStr* Strings;
	UINT16 count;
	LPCSTR LangName;
	LANGID LangID;	
} Languages[] = {
	{ 1252, TABLE(StringsEnglish),	  "English",		MAKELANGID(LANG_ENGLISH,	SUBLANG_ENGLISH_US) },
	{ 1252, TABLE(StringsGerman),	  "German",			MAKELANGID(LANG_GERMAN,		SUBLANG_GERMAN) },
	{ 1251, TABLE(StringsRussian),	  "Russian",		MAKELANGID(LANG_RUSSIAN,	SUBLANG_DEFAULT) },
	{ 1252, TABLE(StringsItalian),	  "Italian",		MAKELANGID(LANG_ITALIAN,	SUBLANG_ITALIAN) },
	{ 1252, TABLE(StringsSpanish),	  "Spanish",		MAKELANGID(LANG_SPANISH,	SUBLANG_SPANISH_MODERN) },
	{ 1252,	TABLE(StringsFrench),	  "French",			MAKELANGID(LANG_FRENCH,		SUBLANG_FRENCH) },
	{ 1250, TABLE(StringsPolish),  	  "Polish",			MAKELANGID(LANG_POLISH,		SUBLANG_DEFAULT) },
	{ 1252, TABLE(StringsPortuguese), "Portuguese (Br)",MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN) },
	{ 1253, TABLE(StringsGreek),	  "Greek",			MAKELANGID(LANG_GREEK,		SUBLANG_DEFAULT) },
	{ 1254, TABLE(StringsTurkish),	  "Turkish",		MAKELANGID(LANG_TURKISH,	SUBLANG_DEFAULT) },
	{ 1252, TABLE(StringsDutch),	  "Dutch",			MAKELANGID(LANG_DUTCH,		SUBLANG_DUTCH) },
	{ 1252, TABLE(StringsDanish),	  "Danish",			MAKELANGID(LANG_DANISH,		SUBLANG_DEFAULT) },
	{ 1250, TABLE(StringsHungarian),  "Hungarian",		MAKELANGID(LANG_HUNGARIAN,	SUBLANG_DEFAULT) },
	{ 1252, TABLE(StringsSwedish),	  "Swedish",		MAKELANGID(LANG_SWEDISH,	SUBLANG_DEFAULT) },
	{ 1252, TABLE(StringsFinnish),	  "Finnish",		MAKELANGID(LANG_FINNISH,	SUBLANG_DEFAULT) },
	{ 1252, TABLE(StringsNorwegian),  "Norwegian",		MAKELANGID(LANG_NORWEGIAN,	SUBLANG_NORWEGIAN_BOKMAL) },
};
		
#pragma pack(pop)

