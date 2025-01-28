#ifndef _SMM_SAMPLER_RINGBUF_H_
#define _SMM_SAMPLER_RINGBUF_H_

#include <inttypes.h>

/*
    Liczba próbek jaką dany blok będzie w stanie zmieścić.
    Informacja o kanale, z jakiej pochodzi dana próbka, jest zakodowana w jej
    indeksie. Chcemy też żeby próbki z danej chwili czasowej zawierały się w
    tylko jednym bloku. Przy wielu wariantach załączeń kanałów wymusza to na nas
    aby rozmiar bufora bloku był podzielny przez NWD(1,2,3) = 6.
    Ważne też, aby liczba ta była parzysta, aby blok mógł być zapełniony próbkami w 100%
    (np. pojemność 3 wymusi użycie 5 bajtów, z czego ostatni będzie wypełniony tylko w połowie)
*/
// #define NEW_SAMPLER__BLOCK_SAMPLE_CAPACITY 600
// #define NEW_SAMPLER__BLOCK_SAMPLE_CAPACITY 504
#define NEW_SAMPLER__BLOCK_SAMPLE_CAPACITY 840
// trzeba tak dobrać tą wartość aby wyrażenie (<wartość>*12/8 + 4) było podzielne przez 8 (żeby dobrze zalignować strukturę samplerd_sample_block_t)
//  oraz <wartość> musi być podzielna przez 1, 2 i 3 aby móc zmieścić wielokrotności próbek dla różnej liczby aktywnych kanałów

/*
    Liczba bajtów, którą powinien mieć bufor aby móc zmieścić
    liczbę próbek zdefiniowaną w poprzednim makrze
*/
#define NEW_SAMPLER__BLOCK_BYTE_CAPACITY NEW_SAMPLER__BLOCK_SAMPLE_CAPACITY * 12 / 8

/*
    Liczba bloków. Wykorzystywane one są do płynnego przesyłania próbek z
    miernika do klienta: Task wykonujący pomiary cyklicznie wypełnia kolejne bloki
    próbkami, a task przesyłający wyniki pomiarów bierze blok, który nie
    jest w danym momencie modyfikowany przez task wykonujący pomiary i go przesyła, bez
    konieczności kopiowania.
    (Zakładam, że im więcej mniejszych bloków tym lepsza gwarancja eliminacji
    race-condition między taskiem zapisującym próbki, a taskiem je przesyłającym.
    Większy całkowity rozmiar bufora też pomaga.)
    Minimalna wartość to 2 btw xD
*/
#define NEW_SAMPLER__BLOCK_COUNT 32

typedef struct {
    uint32_t first_sample_index;
    uint8_t buffer[NEW_SAMPLER__BLOCK_BYTE_CAPACITY];
} samplerd_sample_block_t;

typedef struct {
    samplerd_sample_block_t block_pool[NEW_SAMPLER__BLOCK_COUNT]; // pula bloków
    uint32_t physical_byte;                                       // fizyczny indeks obecnego bajtu w obecnym bloku
    uint32_t logical_block;                                       // logiczny indeks obecnego bloku
    uint32_t num_of_channels;                                     // liczba kanałów jaka jest multipleksowana do wspólnego bufora (rozmiar bufora musi być przez nią podzielny!)
} samplerd_blocks_t;

#ifdef _SMM_SAMPLER_RINGBUF_SHOW_PRODUCER_FUNCTIONS_
void sampler_ringbuf_push_sample(uint8_t *value12bit_l_aligned);

void sampler_ringbuf_restart(uint32_t num_of_channels);
#endif

/**
 * @brief Zwraca wskaźnik na blok o podanym indeksie logicznym. Użytkownik
 *        funkcji jest odpowiedzialny za sprawdzenie, czy dany blok może być
 *        w danym momencie odczytywany. Zaleca się bieżące przetwarzanie bloków
 *        oraz unikanie przetwarzania bloków, które wkrótce będą zapisane.
 *
 * @param logical_block indeks logiczny bloku
 *
 * @returns Wskaźnik na blok (tylko do odczytu)
 */
const samplerd_sample_block_t *sampler_ringbuf_access_block_unchecked(uint32_t logical_block);

/**
 * @brief W trakcie działa samplera zwracany jest indeks obecnie wypełnianego bloku.
 *
 * @returns Indeks obecnie wypełnianego bloku lub dowolna wartość, gdy sampler
 *          jest wyłączony
 */
uint32_t sampler_ringbuf_get_current_logical_block_unchecked();

#endif
