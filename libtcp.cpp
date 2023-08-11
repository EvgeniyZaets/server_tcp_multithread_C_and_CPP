#include "libtcp.h"
#include "vector"

/**
 * @brief Функция для вывода сообщения об ошибке.
 *
 * Функция принимает несколько аргументов: статус, код ошибки, форматированную строку и дополнительные аргументы.
 * Она выводит сообщение об ошибке в стандартный поток ошибок (STDERR_FILENO).
 *
 * @param status Статус программы. Если значение не равно нулю, программа будет завершена с указанным статусом.
 * @param err Код ошибки.
 * @param fmt Форматированная строка.
 * @param ... Дополнительные аргументы, переданные в переменную "ap".
 */
void error(int status, int err, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dprintf(STDERR_FILENO, "%s: ", program_name);
    vdprintf(STDERR_FILENO, fmt, ap);
    va_end(ap);
    if (err)
        dprintf(STDERR_FILENO, ": %s (%d)\n", strerror(err), err);
    if (status)
        exit(status);
}

/**
 * @brief Создает TCP-серверный сокет с использованием заданных параметров.
 *
 * Функция создает TCP-серверный сокет и связывает его с заданным IP-адресом и портом.
 *
 * @param hname IP-адрес или имя хоста. Если значение равно nullptr, сокет будет связан с INADDR_ANY.
 * @param sname Номер порта или имя службы.
 *
 * @return Возвращает созданный TCP-серверный сокет.
 */
SOCKET tcp_server(const char* hname,const char* sname) //ip адрес и порт
{
    struct sockaddr_in local; 		//переменная структуры адреса
    SOCKET s; 						//сокет сервера
    const int on = 1; 				//флаг

    set_address(hname, sname, &local, "tcp"); 		//заполняем структуру адреса
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 			//создаем сокет TCP
    if (!isvalidsock(s)) {							//проверка на ошибку
        error(1, errno, "socket call failed"); 		//ошибка
        return -1;
    }
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) != 0) {	//задаем параметры сокета
        error(1, errno, "setsockopt failed"); 									//ошибка
        return -1;
    }
    if (bind(s, (struct sockaddr*)&local, sizeof(local)) != 0) {		//биндим сокет
        error(1, errno, "bind failed"); 								//ошибка
        return -1;
    }
    if (listen(s, NLISTEN) != 0) {					//слушаем
        error(1, errno, "listen failed"); 			//ошибка
        return -1;
    }
    return s; 		//возвращаем сокет
}

/**
 * @brief Создает TCP-серверный сокет с использованием заданных параметров.
 *
 * Функция создает TCP-серверный сокет и связывает его с заданным IP-адресом, портом и сетевой картой.
 *
 * @param hname IP-адрес или имя хоста. Если значение равно nullptr, сокет будет связан с INADDR_ANY.
 * @param sname Номер порта или имя службы.
 * @param device Название сетевой карты, к которой будет привязан сокет.
 *
 * @return Возвращает созданный TCP-серверный сокет.
 */
SOCKET tcp_server(const char* hname,const char* sname,const char* device) //ip адрес, порт, название сетевой карты
{
   struct sockaddr_in local;
   SOCKET s;
   const int on = 1;

   set_address(hname, sname, &local, "tcp");
   s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (!isvalidsock(s)) {
       error(1, errno, "socket call failed");
       return s;
   }

   if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on))) {
       error(1, errno, "setsockopt failed");
       return s;
   }

   if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, device, strlen(device) + 1)) {
       error(1, errno, "setsockopt failed device");
       return s;
   }

   if (bind(s, (struct sockaddr*)&local, sizeof(local))) {
       error(1, errno, "bind failed");
       return s;
   }

   if (listen(s, NLISTEN)) {
       error(1, errno, "listen failed");
       return s;
   }

   return s;
}


/**
 * @brief Создает TCP-клиентский сокет с использованием заданных параметров.
 *
 * Функция создает TCP-клиентский сокет и устанавливает его адрес на основе заданных IP-адреса и порта.
 *
 * @param hname IP-адрес или имя хоста.
 * @param sname Номер порта или имя службы.
 *
 * @return Возвращает созданный TCP-клиентский сокет.
 */
SOCKET tcp_client(const char* hname,const char* sname) //ip адрес, порт
{
    struct sockaddr_in peer;
    SOCKET s;

    set_address(hname, sname, &peer, "tcp");
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!isvalidsock(s))
    {
        error(1, errno, "socket call failed");
        return -1;
    }
    if (connect(s, (struct sockaddr*)&peer,
        sizeof(peer)))
    {
        error(1, errno, "connect failed");
        return -1;
    }

    return s;
}

/**
 * @brief Создает UDP-серверный сокет с использованием заданных параметров.
 *
 * Функция создает UDP-серверный сокет и связывает его с заданным IP-адресом и портом.
 * Также можно задать название сетевой карты, к которой будет привязан сокет.
 *
 * @param hname IP-адрес или имя хоста. Если значение равно nullptr, сокет будет связан с INADDR_ANY.
 * @param sname Номер порта или имя службы.
 * @param device Название сетевой карты, к которой будет привязан сокет.
 *
 * @return Возвращает созданный UDP-серверный сокет.
 */
SOCKET udp_server(const char* hname,const char* sname, char* device) //ip адрес, порт, название сетевой карты
{
    SOCKET s;
    struct sockaddr_in local;
    const int on = 1;
    set_address(hname, sname, &local, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (!isvalidsock(s))
        error(1, errno, "socket call failed");

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on))) 	//задаем параметры сокета
        error(1, errno, "setsockopt failed"); 								//ошибка

    if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, device, strlen(device)))	//задаем название сетевой карты
        error(1, errno, "setsockopt failed device"); 						//ошибка

    if (bind(s, (struct sockaddr*)&local, sizeof(local)))	//биндим сокет
        error(1, errno, "bind failed"); 					//ошибка

    return s;		//возвращаем сокет
}

/**
 * @brief Создает UDP-серверный сокет с использованием заданных параметров.
 *
 * Функция создает UDP-серверный сокет и связывает его с заданным IP-адресом и портом.
 *
 * @param hname IP-адрес или имя хоста. Если значение равно nullptr, сокет будет связан с INADDR_ANY.
 * @param sname Номер порта или имя службы.
 *
 * @return Возвращает созданный UDP-серверный сокет.
 */
SOCKET udp_server(const char* hname,const char* sname)	//ip адрес, порт
{
    SOCKET s;
    struct sockaddr_in local;
    set_address(hname, sname, &local, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (!isvalidsock(s))
        error(1, errno, "socket call failed");
    if (bind(s, (struct sockaddr*)&local, sizeof(local)))	//биндим сокет
        error(1, errno, "bind failed"); 					//ошибка

    return s; 		//возвращаем сокет
}

/**
 * @brief Создает UDP-клиентский сокет с использованием заданных параметров.
 *
 * Функция создает UDP-клиентский сокет и устанавливает его адрес на основе заданных IP-адреса и порта.
 *
 * @param hname IP-адрес или имя хоста.
 * @param sname Номер порта или имя службы.
 * @param sap Указатель на структуру sockaddr_in, в которую будет записан адрес сокета.
 *
 * @return Возвращает созданный UDP-клиентский сокет.
 */
SOCKET udp_client(const char* hname,const char* sname, struct sockaddr_in* sap)
{
    SOCKET s;
    set_address(hname, sname, sap, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (!isvalidsock(s))
        error(1, errno, "socket call failed");
    return s;
}

/**
 * @brief Устанавливает адрес сокета.
 *
 * Функция устанавливает адрес сокета на основе заданных параметров, включая имя хоста, номер порта и протокол.
 *
 * @param hname Имя хоста или IP-адрес. Если значение равно nullptr, адрес будет установлен на INADDR_ANY.
 * @param sname Номер порта или имя службы.
 * @param sap Указатель на структуру sockaddr_in, в которую будет записан адрес.
 * @param protocol Протокол, который будет использоваться для сокета. Допустимые значения: "tcp" или "udp".
 *
 * @return Возвращает 0 в случае успешного выполнения, -1 в случае ошибки.
 */

void set_address(const char* hname,const char* sname,
    struct sockaddr_in* sap, char* protocol) 		//адрес хоста или его имя, порт, стркуктура адреса, протокол
{
    struct servent* sp;
    struct hostent* hp;
    char* endptr;
    short port;

    memset(sap, 0, sizeof(*sap));
    sap->sin_family = AF_INET;

    if (hname != NULL) {
        if (!inet_pton(AF_INET, hname, &(sap->sin_addr))) {
            hp = gethostbyname(hname);
            if (hp == NULL)
                error(1, 0, "unknown host: %s\n", hname);
            sap->sin_addr = *(struct in_addr*)hp->h_addr_list[0];
        }
    }
    else {
        sap->sin_addr.s_addr = htonl(INADDR_ANY);
    }

    port = strtol(sname, &endptr, 0);
    if (*endptr == '\0') {
        sap->sin_port = htons(port);
    }
    else {
        sp = getservbyname(sname, protocol);
        if (sp == NULL)
            error(1, 0, "unknown service: %s\n", sname);
        sap->sin_port = sp->s_port;
    }
}

/**
 * @brief Читает переменную запись из сокета.
 *
 * Функция читает переменную запись из сокета с использованием заданных параметров.
 *
 * @param fd Дескриптор сокета.
 * @param bp Указатель на буфер, в который будет записана прочитанная запись.
 * @param len Размер буфера.
 *
 * @return Возвращает количество прочитанных байтов в случае успешного выполнения, -1 в случае ошибки.
 */
u_int32_t readvrec(SOCKET fd, char *bp, u_int32_t len)
{
    u_int32_t reclen;
    u_int32_t byte;
    // Прочитать длину записи из первого поля
    byte = readn(fd,(char*)&reclen, sizeof(u_int32_t));
    // Если количество прочитанных байтов не равно ожидаемому размеру u_int32_t,
    // возвращаем соответствующий результат
    if(byte!=sizeof(u_int32_t))
        return byte<0? -1 : 0;
    reclen = htonl(reclen);		// Размер записи преобразуется из сетевого порядка в машинный

    if(reclen > len)
    {
        std::vector<char> buffer(reclen);
        // Читаем запись в буфер размером reclen
        byte = readn(fd, &buffer[0], reclen);
        // Если количество прочитанных байтов не равно ожидаемой длине записи,
        // возвращаем соответствующий результат
        if(byte != reclen)
            return byte < 0 ? -1 : 0;
        // Копируем данные из буфера в bp, ограничиваясь минимальным значением между reclen и len
        std::copy(buffer.begin(), buffer.begin() + std::min(reclen, len), bp);
        if(reclen > len)
        {
            set_errno(EMSGSIZE);
            return -1;
        }
    } else {

    // Прочитать саму запись в bp
    byte = readn(fd, bp, reclen);
    // Если количество прочитанных байтов не равно ожидаемой длине записи,
    // возвращаем соответствующий результат
        if(byte != reclen)
            return byte < 0 ? -1 : 0;
    }
    return byte;
}



/**
 * @brief Функция для чтения записи ровно len байт из сокета.
 *
 * Функция читает ровно len байт из сокета с использованием заданных параметров.
 *
 * @param fd Дескриптор сокета.
 * @param buffer Указатель на буфер, в который будет записано прочитанное содержимое.
 * @param len Размер прочитываемой записи в байтах.
 *
 * @return Возвращает количество прочитанных байтов в случае успешного выполнения, -1 в случае ошибки.
 */
u_int32_t readn(SOCKET fd, char* buffer, u_int32_t len) //сокет, буфер, длина
{
    u_int32_t cnt; 			//переменная остатка байт
    u_int32_t rc; 			//переменная байт что читаем в итерации (шаге)

    cnt = len; 			//задаем начлаьное значение для остатка байт
    while (cnt > 0) 	//если остаток больше нуля
    {
        rc = recv(fd, buffer, cnt, 0); 	//читаем из сокета
        if (rc < 0)						// ошибка прочтения
        {
            if (errno == EINTR)			// если прервано
                continue;				// перезапуск прочтения
            return -1;					// вывод ошибки
        }
        if (rc == 0)					// конец передачи
            return (len - cnt);			// возвращаем количество прочтенных байт
        buffer += rc; 					//добавляем к буферу байты что прочли
        cnt -= rc; 						//вычитаем их того что нужно было прочесь (получаем остаток байт)
    }
    return len; 						//возвращаем длину байт
}

