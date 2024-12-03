#include "server_helper.h"

#pragma region reciever_definitions

static void                 recieve_requests(queue_t * const pqueue, coordination_t * const  coordination, const int server_socket);
static inline unsigned char program_is_running(const coordination_t * const coordination);
static unsigned char        recieved_valid_request(request_t * const prequest, size_t * counter, const int server_socket);
static inline unsigned char would_block(const ssize_t recieved_length);
static int                  recieve_request(request_t * const buffer, sockaddr_in_t * const client_address, socklen_t * const address_length, const int server_socket );
static void                 fill_request(request_t * const prequest, const sockaddr_in_t * const client_address, const size_t id);

#pragma endregion

#pragma region sender_definitions

static void send_processed_request(const request_t * const request, const int server_socket);

#pragma endregion

#pragma region load_balancer_definitions

static int                  find_first_free_worker(queue_t * worker_queues, const size_t workers_count);
static int                  select_worker(queue_t * worker_queues, const size_t workers_count);
static void                 register_to_worker_queues(queue_t * const queues, const size_t workers_count);
static void                 deregister_to_worker_queues(queue_t * const queues, const size_t workers_count);
static void                 balance(queue_t * const input, queue_t * const worker_queues, const size_t workers_count);
static inline unsigned char is_valid_id(const int id, const ssize_t workers_count);

#pragma endregion

#pragma region admin_definitions

static void                  init_listener_socket(int * const listener_socket, int * const error_code, const int port);
static inline unsigned char  is_invalid_socket(const int socket);
static void                  communicate_with_admins(arguments_t * const arguments, const int listener_socket);
static int                   connect_admin(const int listener_socket);
static connection_status_t   communicate_with_admin(arguments_t * const arguments, const int admin_socket);
static inline unsigned char  should_end_program(const connection_status_t connection_status);
static connection_status_t   handle_single_directive(arguments_t * const arguments, const int admin_socket);
static ssize_t               recieve_and_validate_directive(directive_t * const buffer, const int admin_socket);
static connection_status_t   should_halt(ssize_t result, directive_code_t directive_code);
static ssize_t               respond_to_admin(arguments_t * const arguments, const directive_t * const directive, const int admin_socket);
static ssize_t               get_result(arguments_t * const arguments, const  directive_t * const directive);

#pragma endregion

#pragma region worker_definitions

static unsigned char process_single_request(queue_t * queue, request_t * request);
static void          proccess(request_t * request);
static unsigned char validate_request(request_t * request);
static ssize_t       operation_0(const ssize_t * const arguments);
static ssize_t       operation_1(const ssize_t * const arguments);
static ssize_t       operation_2(const ssize_t * const arguments);
static ssize_t       operation_3(const ssize_t * const arguments);

#pragma endregion

#pragma region socket_functions_definitions

static int  create_socket(const int type, const int protocol);
static void init_socket_address(sockaddr_in_t * const server_address, const unsigned short port);
static int  bind_server_socket(const sockaddr_in_t * const server_address, const int server_socket);

#pragma endregion

#pragma region reciever 

void reciever(queue_t * const pqueue,  coordination_t * const coordination, const int server_socket) {
    printf("[DEBUG] Server listening on port: %u ...\n", SERVER_PORT);    
    register_producer(pqueue);
    recieve_requests(pqueue, coordination, server_socket);
    deregister_producer(pqueue);
}

static void recieve_requests(queue_t * const pqueue, coordination_t * const  coordination, const int server_socket) {
    request_t buffer;
    size_t  counter = 0;

    while ( program_is_running(coordination) ) {
        if ( !recieved_valid_request(&buffer, &counter, server_socket) ) {
            continue;
        }
        enqueue(pqueue, &buffer);
    }
}

static inline unsigned char program_is_running(const coordination_t * const coordination) {
    return coordination -> state != COORD_HALT;
} 

static unsigned char recieved_valid_request(request_t * const prequest, size_t * counter, const int server_socket) {
    sockaddr_in_t   client_address;
    socklen_t       address_length = sizeof(sockaddr_in_t);

    if ( !recieve_request(prequest, &client_address, &address_length, server_socket)) {
        return RECIEVE_FAILURE;
    }

    fill_request(prequest, &client_address, ++(*counter));
    printf("[Reciever] Client: %s:%u sent request. Assigned id: %lu\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), prequest -> id);
    return RECIEVE_SUCCESS;
}

static void fill_request(request_t * const prequest, const sockaddr_in_t * const client_address, const size_t id) {
    memcpy(&(prequest -> client_addr), client_address, sizeof(sockaddr_in_t));
    prequest -> id            = id;
    prequest -> flags.handled = 0;
    prequest -> flags.invalid = 0;
    prequest -> flags.stopped = 0;
    prequest -> flags.padding = 0;
    prequest -> result        = 0;
}

static int recieve_request(request_t * const buffer, sockaddr_in_t * const client_address, socklen_t * const address_length, const int server_socket ) {
    const ssize_t recieved_length = recvfrom(server_socket, buffer, sizeof(request_t), MSG_DONTWAIT, (sockaddr_t *)client_address, address_length);
    
    if( would_block(recieved_length) ) {
        // printf("No requests to process. Sleeping ...\n");
        sleep(1);
        return RECIEVE_FAILURE;
    } else if (recieved_length < 0){
        perror("Recieve failed.\n");
        return RECIEVE_FAILURE;
    } else if(recieved_length < sizeof(REQUEST)){
        perror("Recieved request invalid format.\n");
        return RECIEVE_FAILURE;
    }

    return RECIEVE_SUCCESS;
}

static inline unsigned char would_block(const ssize_t recieved_length) {
    return (recieved_length < 0) && (errno == EWOULDBLOCK || errno == EAGAIN);
}


#pragma endregion

#pragma region admin 

void admin(arguments_t * const arguments) {
    int listener_socket;
    int error_code;

    init_listener_socket(&listener_socket, &error_code, LISTENER_PORT);
    if (error_code) {
        handle_halt(arguments -> coordination);
        return;
    }

    communicate_with_admins(arguments, listener_socket);

    shutdown(listener_socket, SHUT_RDWR);
    close(listener_socket);
}

static void init_listener_socket(int * const listener_socket, int * const error_code, const int port) {
    * listener_socket = init_socket(SOCK_STREAM, 0, port); 
    
    if (* listener_socket < 0) {
        perror("Listener socket could not be initialized.\n");
        * error_code = * listener_socket;
        return;
    }

    if (listen(*listener_socket, -1) < 0) {
        perror("Listener socket could not be put in listen mode.\n");
        * error_code = LISTENER_MODE_FAILURE;
        return;
    }

    * error_code = 0;
}

static void communicate_with_admins(arguments_t * const arguments, const int listener_socket) {
    int                 admin_socket;
    connection_status_t connection_status;
    
    while (1) {

        admin_socket = connect_admin(listener_socket);
        if ( is_invalid_socket(admin_socket) ) {
            continue;
        }

        connection_status = communicate_with_admin(arguments, admin_socket); 
        close(admin_socket);
        
        if ( should_end_program(connection_status) )
            break;
    }
}

static int connect_admin(const int listener_socket) {
    sockaddr_in_t admin_address;
    socklen_t     admin_address_len = sizeof(sockaddr_in_t);
    int           admin_socket;

    admin_socket = accept(listener_socket, (sockaddr_t *)&admin_address, &admin_address_len);
    
    if (admin_socket < 0) {
        printf("[Admin] Admin socket accepting failed.\n");
    } else {
        printf("[Admin] Connected from %s:%u\n", inet_ntoa(admin_address.sin_addr), ntohs(admin_address.sin_port));
    }

    return admin_socket;
}

static inline unsigned char is_invalid_socket(const int socket) {
    return socket < 0;
}

static inline unsigned char should_end_program(const connection_status_t connection_status) {
    return connection_status == CONNECTION_END_HALT;
}

static connection_status_t communicate_with_admin(arguments_t * const arguments, const int admin_socket) {
    connection_status_t status;
    while (1) {
        status = handle_single_directive(arguments, admin_socket);
        if (status != CONNECTION_PERSIST) {
            return status;
        }
    }
}

static connection_status_t handle_single_directive(arguments_t * const arguments, const int admin_socket) {
    directive_t buffer;
    ssize_t     recieved_length;
    ssize_t     result;

    recieved_length  = recieve_and_validate_directive(&buffer, admin_socket);
    if (recieved_length == 0)
        return CONNECTION_END_NO_HALT;
    else if (recieved_length < sizeof(directive_t))
        return CONNECTION_PERSIST;

    result = respond_to_admin(arguments, &buffer, admin_socket);
    return should_halt(result, buffer.dir);
}

static ssize_t recieve_and_validate_directive(directive_t * const buffer, const int admin_socket) {
    ssize_t error = INVALID_DIRECTIVE;
    ssize_t recieved_data_length;
    
    printf("HERE\n");
    recieved_data_length = recv(admin_socket, buffer, sizeof(directive_t), 0);
    print_directive(buffer);

    if (recieved_data_length == 0) {
        printf("[Admin] Disconnected.\n");
    } else if (recieved_data_length < sizeof(directive_t)) {
        printf("[Admin] Recieved directive has invalid format. Size : %ld\n", recieved_data_length);
        send(admin_socket, &error, sizeof(ssize_t), 0);        
    }

    return recieved_data_length;
}

static connection_status_t should_halt(ssize_t result, directive_code_t directive_code) {
    unsigned char halt = (result > 0) && (directive_code == DIR_HALT);
    return halt ? CONNECTION_END_HALT : CONNECTION_PERSIST;
} 

static ssize_t respond_to_admin(arguments_t * const arguments, const directive_t * const directive, const int admin_socket) {
    ssize_t result;

    print_directive(directive);
    result = get_result(arguments, directive);
    send(admin_socket, &result, sizeof(ssize_t), 0);
    
    return result;
}

static ssize_t get_result(arguments_t * const arguments, const  directive_t * const directive) {
    ssize_t result;

    result = handle_directive(arguments -> input, arguments -> num, arguments -> coordination, directive);
    
    if (result > 0) {
        printf("[Admin] Directive handled successfully.\n");
    } else  {
        printf("[Admin] Directive handled unsuccessfully.\n");
    }
    
    return result;
}

#pragma endregion

#pragma region socket_helper 

int init_socket(const int type, const int protocol,const  unsigned short port) {
    sockaddr_in_t server_address;
    int           server_socket;
    int           binding_failure;

    init_socket_address(&server_address, port);
    server_socket = create_socket(type, protocol);
    
    if ( server_socket < 0 ) {
        return INVALID_SOCKET;
    }
    
    binding_failure = bind_server_socket( &server_address, server_socket );
    
    if ( binding_failure ) {
        close( server_socket );
        return BINDING_FAILURE;
    }
    
    return server_socket;
}

static void init_socket_address(sockaddr_in_t * const server_address, const unsigned short port) {
    memset( server_address, 0, sizeof(sockaddr_in_t) );
    server_address -> sin_family      = AF_INET;
    server_address -> sin_port        = htons(port);
    server_address -> sin_addr.s_addr = INADDR_ANY;
}

static int create_socket(const int type, const int protocol) {
    int server_socket;
    server_socket = socket(AF_INET, type, protocol);
    if ( server_socket < 0 ) {
        perror("Socket creation failed.\n");
        return INVALID_SOCKET;
    }
    return server_socket;
}

static int bind_server_socket(const sockaddr_in_t * const server_address, const int server_socket) {
    int binding_failure = bind(server_socket, (sockaddr_t *) server_address, sizeof(sockaddr_in_t));
    if (  binding_failure  ) {
        perror("Socket binding failed.\n");
    }
    return binding_failure;
} 

#pragma endregion

#pragma region queues_and_threads 

void safe_dealocalte(void * ptr) {
    if (ptr)
        free(ptr);
    ptr = NULL;
}

void init_queues(QUEUE * const queues, const unsigned short queues_count) {
    for ( int i = 0; i < queues_count; ++i ) {
        if ( i < 2) {
            init_queue( queues + i, QUEUE_CAPACITY_UNLIMITED);
        } else {
            init_queue( queues + i, QUEUE_CAPACITY_DEFAULT );
        }
    }
}

void delete_all_queues(QUEUE * const queues, const unsigned short queues_count) {
    for ( int i = 0; i < queues_count; ++i ) {
        delete_queue( queues + i );
    }
}

void join_all_threads(const pthread_t * const threads, const unsigned short thread_count) {
    for ( int i = 0; i < thread_count; ++i ) {
        pthread_join( threads[i], NULL );
    }
}

int close_program(QUEUE * const queues, const unsigned short queues_count, const int server_socket) {
    delete_all_queues(queues, queues_count);
    return close(server_socket);
}


#pragma endregion

#pragma region sender

void sender(queue_t * const pqueue, const int server_socket) {
    request_t * request; 
    while (1) {
        request = dequeue(pqueue);
        
        if (!request) {
            break;
        }
    
        send_processed_request(request, server_socket);
        safe_dealocalte((void *)request);
    }
}

static void send_processed_request(const request_t * const request, const int server_socket){
    ssize_t      sent_length ;
    sockaddr_t * client_address;  
    
    client_address = (sockaddr_t *)  &(request -> client_addr);
    sent_length    =  sendto(server_socket, request, sizeof(request_t), 0, client_address, sizeof(sockaddr_in_t)) ;
    
    if( sent_length < 0){
        perror("Send failed.\n");
    }
}

#pragma endregion

#pragma region load_balancer

void load_balancer(queue_t * const input,queue_t * const worker_queues) {
    register_to_worker_queues(worker_queues, WORKER_NUMBER);
    balance(input, worker_queues, WORKER_NUMBER);
    deregister_to_worker_queues(worker_queues, WORKER_NUMBER);
}

static void balance(queue_t * const input, queue_t * const worker_queues, const size_t workers_count){
    while(1) {
        request_t * request = dequeue(input);
        if (!request) {
            return;
        }

        int selected_worker = select_worker(worker_queues, workers_count);
        
        printf("[Load Balancer] Request id: %lu assigned to thread %d.\n",request -> id , selected_worker);    
        enqueue(worker_queues + selected_worker, request);
        safe_dealocalte(request);
    }
} 

int select_worker(queue_t * worker_queues, const size_t workers_count) {
    int selected_worker = -1;
    
    while (1) {
        selected_worker = find_first_free_worker(worker_queues, workers_count);
        
        if ( is_valid_id(selected_worker, workers_count) ) {
            return selected_worker;
        }
        
        sleep(1);
    }
}

int find_first_free_worker(queue_t * worker_queues, const size_t workers_count) {
    size_t capacity, size, i;
    queue_flags_t flags;

    for ( i = 0;  i < workers_count; ++i) {
        capacity = worker_queues[i].capacity;
        size     = worker_queues[i].size;
        if ( (!capacity || size < capacity) && !is_queue_stopped(worker_queues + i) ) {
            return i;
        }
    }
    return -1;
}

static inline unsigned char is_valid_id(const int id, const ssize_t workers_count) {
    return (id >= 0) && (id < workers_count);
}

static void register_to_worker_queues(queue_t * const queues, const size_t workers_count) {
    for (int i = 0; i < workers_count; ++i) {
        register_producer(queues + i);
    }
}

static void deregister_to_worker_queues(queue_t * const queues, const size_t workers_count) {
    for (int i = 0; i < workers_count; ++i) {
        deregister_producer(queues + i);
    }

}


#pragma endregion

#pragma region worker

void worker(queue_t * const input, queue_t * const output, const int id) {
    register_producer(output);
    while (1) {
        REQUEST* request = dequeue(input);
        if( !process_single_request(input, request) ){
            break;
        }
        printf("[Worker %d] Worker handled request id: %lu.\n", id, request -> id);
        enqueue(output, request);
        safe_dealocalte((void *)request);
    }
    deregister_producer(output);
}

unsigned char process_single_request(queue_t * queue, request_t * request) {
    if (!request) {
        return 0;
    }

    if (is_queue_stopped(queue)) {
        request -> flags.stopped = 1; 
        return 1;   
    }

    proccess(request);
    return 1;
}

void proccess(request_t * request){
    if ( !validate_request(request) ) {
        return;
    }

    switch (request -> operation) {
        case 0:
            request -> result = operation_0(request -> arguments);
            break;
        case 1:
            request -> result = operation_1(request -> arguments);        
            break;
        case 2:
            request -> result = operation_2(request -> arguments);
            break;
        case 3:
            request -> result = operation_3(request -> arguments);
            break;
        default:
            break;
    }
    request -> flags.handled = 1;
}

unsigned char validate_request(request_t * request) {
    // print_request(request);
    if (request -> flags.stopped)
        return 0;
    if (request -> operation > 3) {
        request -> flags.invalid = 1;
        return 0;
    }

    return 1;
}

ssize_t operation_0(const ssize_t * const arguments) {
    size_t argument = (size_t) arguments[0];
    return (argument * argument++) >> 1;
}

ssize_t operation_1(const ssize_t * const arguments) {
    ssize_t      argument = arguments[0];
    const size_t mask     = argument >> (sizeof(ssize_t) * 0x08 -1);
    return (argument + mask) ^ mask;
}

ssize_t operation_2(const ssize_t * const arguments) {
    ssize_t result = 0;
    for (int i = 0; i < MAX_ARGUMENTS; ++i) {
        result ^= arguments[i];
    }
    return result;
}

ssize_t operation_3(const ssize_t * const arguments) {
    ssize_t result = arguments[0];
    return ~result + 1;
}

#pragma endregion
