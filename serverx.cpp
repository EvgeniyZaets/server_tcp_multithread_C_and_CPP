#include "serverx.h"

/**
 * @brief Функция обработки полученного сообщения и отправки подтверждения.
 *
 * Функция считывает сообщение из сокета fdxin и сохраняет его в поле msgin.
 * Если считанное количество байт равно 0, то функция возвращает результат Break,
 * иначе проверяется условие для переключения на другую сеть. Если считанное количество байт меньше 0,
 * то функция возвращает результат Break. Если LostAck не равно 0, то случайным образом откидывается сообщение,
 * и функция возвращает результат Continue. Увеличивается счетчик cn_in, который отслеживает количество сообщений от клиента.
 * Затем создается подтверждение ACK, которое состоит из символа ACK и признака сообщения, сдвинутого на один символ вправо.
 * Подтверждение отправляется обратно в сокет fdxin. Если отправка не удалась, выводится ошибка.
 * Увеличивается счетчик cn_ack_in, отслеживающий количество отправленных подтверждений.
 * Возвращается результат True.
 *
 * @return Результат операции: Break, Continue или True.
 */
result XIN::Eсho()  //полученное сообщение сохраняется в поле msgin
{
    int byte = readvrec(fdxin,msgin,size_in);
    if ( byte == 0 )
    {
        loger(WARNING, errno, "клиент порт IN отсоединился");//лог
        //error( 0, errno, "клиент порт IN отсоединился\n" );
        return Break;
    }
     /*здесь должно быть переключение на другую сеть*/
    if ( byte < 0 )
    {
        loger(ERROR, 0, "ошибка вызова recv - recvrecmsg");//лог
        //error( 0, errno, "ошибка вызова recv\n" );
        return Break;
    }
    
    cn_in +=1;                                          //счетчик всего сообщений от клиента
    char msg[ACKSZ];
    memmove(msg+1, msgin, COOKIESZ);                      //сдвигаем в буфере признак на один символ вправо
    msg[ 0 ] = ACK;                                     //добавляем в начало символ ACK

    byte = send(fdxin, msg, ACKSZ, 0 );
    if (  byte < 0 )                                    //отправляем его обратно
    {
        loger(ERROR, 0, "ошибка вызова send");//лог
    }
    cn_ack_in +=1;
    return True;
}

/**
 * @brief Возвращает указатель на свободную запись сообщения.
 *
 * Функция ищет свободную запись сообщения в пуле mr.
 * Если найдена свободная запись, то возвращается указатель на нее.
 * Если свободная запись не найдена, выводится ошибка и возвращается nullptr.
 *
 * @return Указатель на свободную запись сообщения или nullptr.
 */
msgrec_t *XOUT::getfreerec()
{
    msgrec_t *data;
        for (data=mr;data<mr+MRSZ; data++)
            if(data->pkt.len==static_cast<u_int32_t>(-1)) /*Запись свободна ?*/
                return data;
    loger(ERROR, 0, "getfreerec: исчерпан пул записей сообщений ");//лог
    return nullptr;
}

/**
 * @brief Ищет запись сообщения по заданному идентификатору.
 *
 * Функция ищет запись сообщения в пуле mr по заданному идентификатору.
 * Если найдена запись с указанным идентификатором и длина сообщения не равна -1,
 * то возвращается указатель на эту запись.
 * Если запись не найдена, выводится отладочное сообщение и возвращается nullptr.
 *
 * @param mid Идентификатор сообщения.
 * @return Указатель на запись сообщения или nullptr.
 */
msgrec_t *XOUT::findmsgrec(u_int32_t mid)
{
    msgrec_t *data;
        for (data=mr;data<mr+MRSZ; data++)
            if(data->pkt.len != static_cast<u_int32_t>(-1) && data->pkt.cookie==mid)
                return data;
    loger(DEBUG, 0, "findmsgrec: нет сообщения соответсвующего ACK %d\n", mid);//лог
    return nullptr;
}

/**
 * @brief Освобождает запись сообщения.
 *
 * Функция проверяет, что запись сообщения уже освобождена.
 * Если запись сообщения уже освобождена, выводится ошибка.
 * Затем устанавливается длина записи сообщения в -1.
 *
 * @param data Указатель на запись сообщения.
 */
void XOUT::freemsgrec(msgrec_t * data)
{
    if ( data->pkt.len == static_cast<u_int32_t>(-1) ){
        loger(ERROR, 0, "freemsgrec: запись сообщения уже освобождена");//лог
    }
    data->pkt.len = -1;
}

/**
 * @brief Отбрасывает сообщение.
 *
 * Функция отбрасывает сообщение и освобождает запись сообщения.
 * Выводится отладочное сообщение о потерянном ACK.
 *
 * @param data Указатель на запись сообщения.
 */
void XOUT::drop(msgrec_t * data)
{
    loger(DEBUG, 0, "Сообщение отбрасывается: потерян ACK ");//лог
    freemsgrec( data );
}

/**
 * @brief Отправляет сообщение.
 *
 * Функция отправляет сообщение, передавая массив msg и его размер size.
 * Получает свободную запись сообщения с помощью функции getfreerec().
 * Заполняет структуру сообщения данными из массива msg.
 * Устанавливает идентификатор сообщения и длину сообщения.
 * Отправляет сообщение через сокет fdxout.
 * Если отправка не удалась, возвращает ошибку и освобождает запись сообщения.
 * Если отправка удалась, сохраняет идентификатор таймера на сообщение и запускает таймер статистики.
 * Возвращает результат True.
 *
 * @param msg Массив сообщения.
 * @param size Размер сообщения.
 * @return Результат операции: Error или True.
 */
result XOUT::SendMsg(char msg[], int size)//передаем массив и его размер
{
    msgrec_t* data_out;
    data_out = getfreerec();
    //заполняем структуру сообщения
    strncpy(data_out->pkt.buf, msg, size);
    data_out->pkt.buf[size-1]='\0';
    data_out->pkt.cookie = msgid++;
    data_out->pkt.len = htonl(sizeof(u_int32_t)+size);
    int byte = send(fdxout, &data_out->pkt, 2*sizeof(u_int32_t)+size, 0);
    if(byte < 0)
    {
        //неудачная отправка сообщения
        //возвращаем сообщение в список свободных
        freemsgrec(data_out);
        loger(ERROR, 0, "ошибка соединения");  //лог
        return Error;
    }
    else
    {
        //обрабатываем удачную отправку сообщения
        //сохраняем идентификатор таймера на сообщение
        data_out->id=TimeXout.timeout([&](){drop(data_out);},T1);
        //запускаем таймер статистики между отправкой и подтверждением о получении
        gettimeofday(&data_out->start, NULL);
        return True;
    }
}

/**
 * @brief Принимает подтверждение сообщения.
 *
 * Функция принимает подтверждение сообщения через сокет fdxout.
 * Если принято 0 байт, выводится предупреждение и возвращается результат Break.
 * Если принято отрицательное количество байт, выводится ошибка.
 * Если число принятых байт меньше размера подтверждения, возвращается результат Continue.
 * Проверяется полученное подтверждение на соответствие ACK.
 * Увеличивается счетчик подтвержденных сообщений cn_ack_out.
 * Извлекается идентификатор сообщения из подтверждения.
 * Ищется запись сообщения по идентификатору.
 * Если запись сообщения найдена, останавливается таймер, освобождается запись сообщения и возвращается результат True.
 * Если запись сообщения не найдена, возвращается результат Error.
 *
 * @return Результат операции: Break, Continue, True или Error.
 */
result XOUT::RecvAck()
{
    int byte = recv (fdxout, ack+cnt, static_cast<int>(ACKSZ)-cnt,0);
    if (byte == 0 )
    {
        loger(WARNING, errno, "клиент порт OUT отсоединился");//лог
        error (0,errno,"клиент порт OUT отсоединился\n");
        return Break;
    }
    else if(byte <0)
    {
        loger(ERROR, errno, "ошибка вызова recv - recvackmsg");//лог
        error ( 0,errno,"ошибка вызова recv\n");
    }
    //Целое сообщение ?
    if((cnt+=byte) < static_cast<int>(ACKSZ))                 //если число байт меньше ACKSZ то возвращаемя к началу цикла
        return Continue;                                      //нет еще подождем
    cnt=0;
    //проверям полученныен данные
    if(ack[0]!=ACK)                                     //точно ли это ACK
    {
        loger(WARNING, 0, "неверное подтверждение");//лог
        return Continue;
    }
    cn_ack_out+=1;                                          //увеличиваем счетчик подтвержденных сообщений
    memcpy(&mid,ack+1,sizeof(u_int32_t));               //извелкаем признак сообщения

    msgrec_t* data_out = findmsgrec(mid);                         // находит сообщение по идентификатору*/
    if(data_out!=nullptr)                                          // если указатель на сообщеие найден верно
    {
        gettimeofday(&end, NULL);
        long seconds = (end.tv_sec - data_out->start.tv_sec);
        long micros = ((seconds * 1000000) + end.tv_usec) - (data_out->start.tv_usec);
        setTimeOut(micros);
        setTimeOutMax(micros);
        TimeXout.unitimeout(data_out->id);          //отмены таймера
        freemsgrec(data_out);                       //удалить сохраненное сообщение - освобождаем msgrec_t.
        return True;                               
    } else return Error;
}

/**
 * @brief Запускает сервер.
 *
 * Функция запускает сервер на устройстве с заданным именем server_name и IP-адресом ip.
 * Создает сокеты для прослушивания портов OUT и IN.
 * Устанавливает маску на чтение для всех сокетов и максимум.
 * В бесконечном цикле выполняет следующие действия:
 * - Если флаг sock_in_flag и флаг sock_out_flag равны true, запускает функцию ClientHandler(),
 *   передавая сокеты client_in и client_out.
 * - Устанавливает маску на чтение для всех сокетов и вызывает функцию tselect() для ожидания событий.
 * - Если tselect() возвращает отрицательное значение, выводит ошибку и продолжает цикл.
 * - Если tselect() возвращает 0, выводит сообщение о том, что нет событий, и завершает функцию.
 * - Если сокет server_out готов для чтения и флаг sock_out_flag равен false,
 *   вызывает функцию AcceptToServer() для принятия клиента на сокет server_out
 *   и установки флага sock_out_flag в true.
 * - Если сокет server_in готов для чтения и флаг sock_in_flag равен false,
 *   вызывает функцию AcceptToServer() для принятия клиента на сокет server_in
 *   и установки флага sock_in_flag в true.
 *
 * @return Отсутствует.
 */
void ServerXOI::Start()
{
    *scout << "Start server on device: " << server_name <<" ip " << ip << "\n";
    fd_set allreads;                    //исходная маска на чтение
    fd_set readmask;                    //маска на чтение
    int maxfd1;                         //максимум
    loger(INFO, 0, "Запуск прослушивания %s",server_name);
    server_out = tcp_server(ip,port_out, server_name);
    server_in = tcp_server(ip,port_in, server_name);

    loger(INFO, 0, "Server started on port OUT(%s) and IN(%s) device : %s",port_out,port_in, server_name);
    *scout << "Server listen on port OUT (" << port_out << ") and IN(" << port_in << ") device : " << server_name << "\n";

    FD_ZERO(&allreads);
    FD_SET(server_out,&allreads);                           //добавляем порт OUT
    FD_SET(server_in,&allreads);                            //добавляем порт IN
    maxfd1 = (std::max(server_out, server_in) + 1);         //считаем максимум + 1
    while(true){
        if(sock_in_flag==true && sock_out_flag==true)
        {
            *scout << "запуск функции клиента и передача сокетов\n";
            loger(INFO, 0, "Запуск обработки клиента");
            fdxin = client_in;
            printf("fdxin %d\n",fdxin);
            fdxout = client_out;
            printf("fdxin %d\n",fdxout);
            ClientHandler();
            sock_out_flag = false;
            sock_in_flag = false;
        }
        readmask = allreads;
        int byte = TimeXout.tselect(maxfd1,&readmask,NULL,NULL);
        if(byte<0)
        {
            loger(ERROR, 0, "ошибка вызова tselect");
            *scout << "ошибка вызова tselect\n";
            continue;
        }
        if ( byte == 0 )
        {
            loger(ERROR, 0, "tselect говорит что нет событий");
            *scout << "tselect говорит что нет событий\n";
            return;
        }
        if(FD_ISSET(server_out, &readmask) && !sock_out_flag)
        {
            AcceptToServer(server_out, &sock_out_flag, &client_out);


        }
        if(FD_ISSET(server_in, &readmask) && !sock_in_flag)
        {
            AcceptToServer(server_in, &sock_in_flag, &client_in);


        }
    }
}

/**
 * @brief Обнуление счетчиков.
 */
void ServerXOI::init_cnt()
{
    msgid = 0;
    Lost =0;
    cnt = 0;
    cn_in = 0;
    cn_ack_out = 0;
    cn_ack_in = 0;
    time_out = 0;
    time_out_max = 0;
    sr_time = 0;
}

/**
 * @brief Обрабатывает клиента.
 *
 * Функция обрабатывает клиента, принимая и отправляя сообщения через сокеты fdxin и fdxout.
 * Инициализирует маску на чтение для всех сокетов и максимум.
 * В бесконечном цикле выполняет следующие действия:
 * - Формирует сообщение из случайных символов.
 * - Если отправка сообщения успешна, устанавливает таймер на повторную отправку через 20 мсек.
 * - Печатает статистику, если прошло TSTAT мсек.
 * - Устанавливает маску на чтение для всех сокетов и вызывает функцию tselect() для ожидания событий.
 * - Если tselect() возвращает отрицательное значение, выводит ошибку и продолжает цикл.
 * - Если tselect() возвращает 0, выводит сообщение о том, что нет событий, и завершает функцию.
 * - Если сокет fdxout готов для чтения, вызывает функцию RecvAck() для получения подтверждений от клиента порта OUT.
 * - Если сокет fdxin готов для чтения, вызывает функцию Echo() для получения данных от клиента и отправки подтверждений IN.
 * - Если функция Echo() возвращает результат Break, завершает функцию.
 * - Если функция Echo() возвращает результат Continue, продолжает цикл.
 * - Если функция Echo() возвращает результат True, копирует полученное сообщение в переменную buf.
 * - Закрывает сокеты client_in и client_out.
 *
 * @return Отсутствует.
 */
void ServerXOI::ClientHandler()
{
    *scout << "Начало обработки клиента \n";

    fd_set allreads;                    //макска набора дескрипторов на чтение исходная
    fd_set readmask;                    //макска набора дескрипторов на чтение рабочая
    int maxfd1;                         //максимум по маскам набора
    //инициализируем маски на чтение
    FD_ZERO(&allreads);                             //обнуляем маску
    FD_SET(fdxout,&allreads);                       //добавляем порт IN
    FD_SET(fdxin,&allreads);                        //добавляем порт OUT
    FD_SET(0,&allreads);                            //добавляем ввод с консоли
    maxfd1 = std::max(fdxin, fdxout) + 1;           //считаем максимум

    //формируем сообщение и делаем каждый раз повтор сообщения
    std::function<void()> sendrev = [&](){
        char buf[BUF];
        srand(time(NULL));
        for (int i = 0; i < BUF-1; i++)                        //формируем строку из случайных символов для отладки
        {
            int random = rand() % 52;                       //26 заглавных и 26 строчных букв
            if (random < 26) buf[i] = random + 65;         //заглавные буквы
            else buf[i] = random + 71;                    //строчные буквы
        }
        buf[BUF-1] = '\0';

        //если успешно делаем повтор через еще 20 млсек
        if(SendMsg(buf,BUF) != Error){
            TimeXout.timeout(sendrev,T0);
            loger(DEBUG, 0, "отправлено сообщение");
        };
    };


    //взводим таймер на первую отправку сообщения через 20 млсек
    TimeXout.timeout(sendrev,T0);

    while(1)
    {
        //печать статистики
        if(msgid == TSTAT/T0){        //2500 мс / 20 мс = 125
            statistic();
        }

        readmask = allreads; //сохраняем маску на чтение
        int byte = TimeXout.tselect(maxfd1,&readmask,NULL,NULL);
        if(byte<0)
        {
            loger(ERROR, 0, "ошибка вызова tselect");//лог
            error(1,0,"ошибка вызова tselect");
        }
        if ( byte == 0 ) //возврат нуля не свидетесльство тайм аута, а свидетесльство ошибки тайм-ауты обрабатываются внутри
        {
            loger(ERROR, 0, "tselect говорит что нет событий");//лог
            error(1,0,"tselect говорит что нет событий\n");
        }

        //получение подтверждений от клиента порта OUT

        if(FD_ISSET(fdxout,&readmask))
        {
            result res =RecvAck();
            if(res == Break) break;
            if(res == Continue) continue;
            if(res == Error) {
                loger(ERROR, 0, "RecvAck ошибка");
                continue;
            }

        }

        //получение данных от клиента и отправка подтверждений IN
        if(FD_ISSET(fdxin,&readmask))
        {
            //создаем переменную для сообщения
            char buf[(sizeof(u_int32_t)*2)+BUF];
            //передаем размер переменной
            size_in = sizeof(buf);
            //считываем сообщение и отправляем подтверждение
            result res = Eсho();
            if(res == Break) break;
            if(res == Continue) continue;
            if(res == True) {
            //копируем полученное сообщение
            memmove(buf, msgin, size_in);
            }
        }

    }
    loger(WARNING, 0, "закрываем процесс обработки клиента ");//лог
    *scout << "закрываем процесс обработки клиента " << "\n";
    close(client_in);
    close(client_out);
}

/**
 * @brief Выводит статистику и сохраняет в файл логов.
 *
 * @param safeCout Объект SafeCout для безопасного вывода.
 * @param device Указатель на имя сетевой карты.
 *
 * Этот метод выводит статистику и сохраняет ее в файл логов. Он использует переданный объект SafeCout для безопасного вывода.
 * Метод собирает данные статистики в двумерный вектор и передает его в функцию createTable для создания таблицы.
 * Затем метод выводит данные статистики в файл логов с использованием функции loger.
 * После вывода статистики, метод обнуляет счетчики.
 *
 * Пример использования:
 * @code
 * SafeCout safeCout;
 * char* device = "eth0";
 * Message message;
 * message.statistic(safeCout, device);
 * @endcode
 *
 * В этом примере создается объект SafeCout для безопасного вывода, указывается имя сетевой карты и создается объект класса Message.
 * Метод statistic используется для вывода статистики и сохранения ее в файл логов.
 * Сначала данные статистики собираются в двумерный вектор, затем создается таблица с помощью функции createTable, и данные выводятся с использованием объекта SafeCout.
 * Затем метод выводит данные статистики в файл логов с использованием функции loger.
 * Наконец, метод обнуляет счетчики с помощью метода init_cnt.
 */
void ServerXOI::statistic()
{
    
    sr_time=time_out/msgid;
    std::vector<std::vector<std::string>> tableData = {
        {"Название сетевой карты", server_name, " "},
        {"Параметр", "Значение", "Размерность"},
        {"Статистика за последние", std::to_string(TSTAT / 1000), "сек"},
        {"1. Отправлено всего сообщений клиенту OUT", std::to_string(msgid), "шт."},
        {"2. Получено всего сообщений от клиента IN", std::to_string(cn_in), "шт."},
        {"3. Получено получено ack в OUT", std::to_string(cn_ack_out), "шт."},
        {"4. отправлено ack в IN", std::to_string(cn_ack_in), "шт."},
        {"5. потеряно ack в OUT", std::to_string(msgid - cn_ack_out), "шт."},
        {"6. неподтверждено ack в IN", std::to_string(Lost), "шт."},
        {"7. время от отправки сообщения до получения акта общее ", std::to_string(msgid), "мкс(us)"},
        {"8. время от отправки сообщения до получения акта общее среднее ", std::to_string(msgid), "мкс(us)"},
        {"9. время от отправки сообщения до получения акта общее максимальное ", std::to_string(msgid), "мкс(us)"}
    };

    loger(INFO, 0, "За последние %d сек:", (TSTAT/1000));//лог
    loger(INFO, 0, "отправлено всего сообщений клиенту OUT (%d) шт.",msgid);//лог
    loger(INFO, 0, "получено всего сообщений от клиента IN (%d) шт.",cn_in);//лог
    loger(INFO, 0, "получено ack в OUT (%d) шт.",cn_ack_out);//лог
    loger(INFO, 0, "отправлено ack в IN (%d) шт.",cn_ack_in);//лог
    loger(INFO, 0, "потеряно ack в OUT (%d) шт.",(msgid-cn_ack_out));//лог
    loger(INFO, 0, "время от отправки сообщения до получения акта общее (%d) microsec",time_out);//лог
    loger(INFO, 0, "время от отправки сообщения до получения акта общее среднее (%d) microsec",sr_time);//лог
    loger(INFO, 0, "время от отправки сообщения до получения акта общее максимальное (%d) microsec",(time_out_max));//лог

    createTable(tableData, *scout);
    init_cnt();//обнуляем счетчики
}

void ServerXOI::AcceptToServer(int server, bool *sock_flag, int *client)
{
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    *client = accept(server, (struct sockaddr*)&addr, &addr_size);
    if (!isvalidsock(*client))
    {
        loger(ERROR, errno, "accept failed");
        //error(0, errno, "accept failed\n");
        close(*client);
        *client = socket(AF_INET, SOCK_STREAM, 0);
        if(*client < 0)
        {
            loger(ERROR, errno, "socket creation failed");
            //error(0, errno, "socket creation failed\n");
        }
    }else{
        loger(INFO, 0, "New connection , socket fd is %d , ip is : %s , port : %d",
        *client,
        inet_ntoa(addr.sin_addr),
        ntohs(addr.sin_port));
        *scout << "New connection , socket fd is (" << *client
               << ") , ip is (" << inet_ntoa(addr.sin_addr)
               << ") , port : (" << ntohs(addr.sin_port) << ")" << "\n";
        *sock_flag = true;
    }
    return;
}
