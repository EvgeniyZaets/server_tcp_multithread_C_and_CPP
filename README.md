# server_tcp_multithread_CPP
Шаблон для многопоточного сервера на С++ для Linux
Проект переписан с языка С на С++, разделен на несколько файлов, созданы наборы классов (Сервер, Пул потоков, Менеджер таймеров, Потокобезопастный вывод в консоль, добавлена из других моих проектов система лонгирования "XLOGER" и библиотека Linyx-сокетов "xlibsocket". Каждый экземпляр сервера запускается в отдельном потоке на конкретной сетевой карте и работает по двум портам с одним конкретным клиентом.

TO DO: Позже будет добавлена реализация класса сервер для обработки по одному порту на каждого клиента в пуле потоков.
ссылка на документацию: https://evgeniyzaets.github.io/server_tcp_multithread_C_and_CPP/html/index.html.
