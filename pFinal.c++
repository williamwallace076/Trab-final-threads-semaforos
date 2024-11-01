#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <random>
#include <condition_variable>
#include <atomic>

using namespace std;

// Variáveis globais
int num_caixas;
int caixas_disponiveis;
mutex caixas_mutex;  // Mutex para proteger o número de caixas disponíveis
condition_variable caixas_cond;  // Variável de condição para controlar os clientes
mutex cout_mutex;  // Mutex para proteger a saída no console
int clientes_na_fila = 0;
vector<int> tempo_espera;  // Armazena o tempo de espera de cada cliente

mutex relatorio_mutex;  // Mutex para proteger o relatório
atomic<int> clientes_insatisfeitos{0};  // Contagem de clientes insatisfeitos
atomic<bool> continuar_rodando{true};  // Variável para controlar se o programa deve continuar rodando

// Função para gerar tempo de atendimento aleatório (entre 6 e 10 segundos)
int gerar_tempo_atendimento() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(6, 10);  // Gera valores entre 6 e 10
    return dist(gen);
}

// Função para simular o atendimento de um cliente no caixa
void cliente_ao_caixa(int cliente_id) {
    auto tempo_chegada = chrono::steady_clock::now();  // Tempo de chegada do cliente

    {
        lock_guard<mutex> lock(cout_mutex);  // Protege a escrita no console
        cout << "Cliente " << cliente_id << " está aguardando um caixa.\n";
    }

    clientes_na_fila++;  // Incrementa o número de clientes aguardando

    // Cliente aguarda até que um caixa fique disponível
    {
        unique_lock<mutex> lock(caixas_mutex);
        caixas_cond.wait(lock, []{ return caixas_disponiveis > 0; });  // Espera até que haja caixa disponível
        caixas_disponiveis--;  // Caixa ocupado
    }

    clientes_na_fila--;  // Decrementa o número de clientes na fila

    auto tempo_atendimento = chrono::steady_clock::now();  // Tempo em que o cliente é atendido
    {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Cliente " << cliente_id << " está sendo atendido no caixa.\n";
    }

    // Calcula o tempo de espera do cliente
    int tempo_espera_cliente = chrono::duration_cast<chrono::seconds>(tempo_atendimento - tempo_chegada).count();
    {
        lock_guard<mutex> lock(relatorio_mutex);  // Protege o acesso à lista de tempos de espera
        tempo_espera.push_back(tempo_espera_cliente);
    }

    // Verifica se o cliente está insatisfeito (esperou mais de 20 segundos)
    if (tempo_espera_cliente > 20) {
        clientes_insatisfeitos++;  // Incrementa a contagem de clientes insatisfeitos
        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Cliente " << cliente_id << " está insatisfeito (esperou " << tempo_espera_cliente << " segundos).\n";
        }
    }

    // Simula o tempo de atendimento (aleatório entre 6 e 10 segundos)
    int tempo_atendimento_variavel = gerar_tempo_atendimento();
    this_thread::sleep_for(chrono::seconds(tempo_atendimento_variavel));

    {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Cliente " << cliente_id << " terminou o atendimento (atendimento durou "
                  << tempo_atendimento_variavel << " segundos) e liberou o caixa.\n";
    }

    // Libera o caixa para outro cliente
    {
        lock_guard<mutex> lock(caixas_mutex);
        caixas_disponiveis++;
        caixas_cond.notify_one();  // Notifica um cliente na fila
    }
}

// Função que permite ao usuário visualizar o estado dos caixas
void exibir_estado() {
    while (continuar_rodando) {  // Checa continuamente se deve continuar rodando
        this_thread::sleep_for(chrono::seconds(1));  // Atualiza o estado a cada segundo
        lock_guard<mutex> lock(cout_mutex);  // Protege a escrita no console
        cout << "[Estado Atual] Caixas ocupados: " << num_caixas - caixas_disponiveis
                  << " / " << num_caixas << " | Clientes na fila: " << clientes_na_fila << "\n";
    }
}

// Função que permite ao usuário abrir ou fechar caixas dinamicamente
void gerenciar_caixas() {
    while (continuar_rodando) {
        int escolha;
        cout << "Digite 1 para abrir um novo caixa, 2 para fechar um caixa: \n";
        cin >> escolha;

        if (escolha == 1) {
            // Abre um novo caixa
            lock_guard<mutex> lock(caixas_mutex);
            if (num_caixas < 10) {  // Limite arbitrário de 10 caixas
                num_caixas++;
                caixas_disponiveis++;
                caixas_cond.notify_one();  // Notifica clientes aguardando
                cout << "Um novo caixa foi aberto! Agora temos " << num_caixas << " caixas disponíveis.\n";
            } else {
                cout << "Já atingimos o número máximo de caixas (10).\n";
            }
        } else if (escolha == 2) {
            // Fecha um caixa
            lock_guard<mutex> lock(caixas_mutex);
            if (num_caixas > 1) {
                num_caixas--;
                caixas_disponiveis--;
                cout << "Um caixa foi fechado! Agora temos " << num_caixas << " caixas disponíveis.\n";
            } else {
                cout << "Não é possível fechar mais caixas (mínimo de 1).\n";
            }
        }
    }
}

// Função para gerar o relatório final
void gerar_relatorio() {
    lock_guard<mutex> lock(relatorio_mutex);
    int total_tempo_espera = 0;

    cout << "\nRelatório Final:\n";
    cout << "Número total de clientes atendidos: " << tempo_espera.size() << "\n";
    cout << "Número de clientes insatisfeitos (espera > 20 segundos): " << clientes_insatisfeitos << "\n";

    // Mostra o tempo de espera de cada cliente
    cout << "Tempos de espera dos clientes:\n";
    for (size_t i = 0; i < tempo_espera.size(); i++) {
        cout << "Cliente " << i << ": " << tempo_espera[i] << " segundos\n";
        total_tempo_espera += tempo_espera[i];
    }

    double media_tempo_espera = static_cast<double>(total_tempo_espera) / tempo_espera.size();
    cout << "\nTempo médio de espera: " << media_tempo_espera << " segundos\n";
}

int main() {
    int num_clientes;

    // Solicita que o usuário defina o número de caixas e clientes
    cout << "Digite o número de caixas disponíveis no supermercado: ";
    cin >> num_caixas;
    cout << "Digite o número total de clientes que irão ao supermercado: ";
    cin >> num_clientes;

    // Inicializa o número de caixas disponíveis
    caixas_disponiveis = num_caixas;

    // Vetor para armazenar as threads dos clientes
    vector<thread> clientes;

    // Thread para exibir o estado do sistema em tempo real
    thread estado_thread(exibir_estado);

    // Thread para o usuário gerenciar os caixas dinamicamente
    thread gerenciar_thread(gerenciar_caixas);

    // Cria threads representando os clientes
    for (int i = 0; i < num_clientes; ++i) {
        clientes.push_back(thread(cliente_ao_caixa, i));
        this_thread::sleep_for(chrono::milliseconds(2000));  // Intervalo de chegada dos clientes
    }

    // Aguarda que todas as threads de clientes terminem
    for (auto& cliente : clientes) {
        cliente.join();
    }

    // Gera o relatório final
    gerar_relatorio();

    // Como todos os clientes foram atendidos, encerramos a thread de exibição do estado
    continuar_rodando = false;
    estado_thread.join();

    // Aguarda que o gerenciamento de caixas termine
    gerenciar_thread.join();

    cout << "Todos os clientes foram atendidos.\n";
    return 0;
}
