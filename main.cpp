#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <semaphore>
#include <atomic>
#include <random>

using namespace std;

// Variáveis globais
int num_caixas;  // Número de caixas ativos
atomic<int> caixas_ocupados{0};  // Contador de caixas ocupados
counting_semaphore<10> semaforo_caixas(0);  // Semáforo para controlar os caixas
mutex cout_mutex;  // Mutex para proteger a saída no console
vector<int> tempo_espera;  // Armazena o tempo de espera de cada cliente
atomic<int> clientes_insatisfeitos{0};  // Contagem de clientes insatisfeitos
atomic<bool> continuar_rodando{true};  // Variável para controlar se o programa deve continuar rodando

// Função para gerar tempo de atendimento aleatório (entre 6 e 10 segundos)
int gerar_tempo_atendimento() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(6, 10);
    return dist(gen);
}

// Função para simular o atendimento de um cliente no caixa
void cliente_ao_caixa(int cliente_id) {
    auto tempo_chegada = chrono::steady_clock::now();

    {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Cliente " << cliente_id << " está aguardando um caixa.\n";
    }

    // Cliente aguarda até que um caixa fique disponível
    semaforo_caixas.acquire();  // Bloqueia o recurso (caixa)

    caixas_ocupados++;  // Incrementa o contador de caixas ocupados
    auto tempo_atendimento = chrono::steady_clock::now();
    {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Cliente " << cliente_id << " está sendo atendido no caixa.\n";
        cout << "Caixas ocupados: " << caixas_ocupados.load() << " / " << num_caixas << "\n";
    }

    // Calcula o tempo de espera do cliente
    int tempo_espera_cliente = chrono::duration_cast<chrono::seconds>(tempo_atendimento - tempo_chegada).count();
    {
        lock_guard<mutex> lock(cout_mutex);
        tempo_espera.push_back(tempo_espera_cliente);
    }

    // Verifica se o cliente está insatisfeito (esperou mais de 20 segundos)
    if (tempo_espera_cliente > 20) {
        clientes_insatisfeitos++;
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

    caixas_ocupados--;  // Decrementa o contador de caixas ocupados
    semaforo_caixas.release();  // Libera o caixa (recurso) para outro cliente
}

// Função para gerar o relatório final
void gerar_relatorio() {
    lock_guard<mutex> lock(cout_mutex);
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

// Função para gerenciar o número de caixas em tempo real
void gerenciar_caixas() {
    while (continuar_rodando) {
        int opcao;
        cout << "\nDigite 1 para aumentar o número de caixas, 2 para diminuir: ";
        cin >> opcao;

        if (opcao == 1) {
            if (num_caixas < 10) {
                semaforo_caixas.release(1);  // Libera mais um caixa
                num_caixas++;
                {
                    lock_guard<mutex> lock(cout_mutex);
                    cout << "Caixa adicional aberto. Caixas agora: " << num_caixas << "\n";
                }
            } else {
                cout << "O número máximo de caixas (10) já foi atingido.\n";
            }
        } else if (opcao == 2) {
            if (num_caixas > 1) {
                semaforo_caixas.acquire();  // Retira um caixa
                num_caixas--;
                {
                    lock_guard<mutex> lock(cout_mutex);
                    cout << "Um caixa foi fechado. Caixas agora: " << num_caixas << "\n";
                }
            } else {
                cout << "Não é possível fechar mais caixas. Deve haver pelo menos 1 caixa disponível.\n";
            }
        }
    }
}

int main() {
    int num_clientes;

    // Solicita que o usuário defina o número de caixas e clientes
    cout << "Digite o número de caixas disponíveis no supermercado (máximo de 10): ";
    cin >> num_caixas;
    cout << "Digite o número total de clientes que irão ao supermercado: ";
    cin >> num_clientes;

    // Inicializa o número de caixas disponíveis no semáforo
    semaforo_caixas.release(num_caixas);

    // Inicia a thread para gerenciar caixas em tempo real
    thread thread_gerenciar(gerenciar_caixas);

    // Vetor para armazenar as threads dos clientes
    vector<thread> clientes;

    // Cria threads representando os clientes
    for (int i = 0; i < num_clientes; ++i) {
        clientes.push_back(thread(cliente_ao_caixa, i));
        this_thread::sleep_for(chrono::milliseconds(2000));  // Intervalo de chegada dos clientes
    }

    // Aguarda que todas as threads de clientes terminem
    for (auto& cliente : clientes) {
        cliente.join();
    }

    // Encerra a execução do gerenciador de caixas
    continuar_rodando = false;
    thread_gerenciar.join();

    // Gera o relatório final
    gerar_relatorio();

    cout << "Todos os clientes foram atendidos. Encerrando o sistema.\n";
    return 0;
}
