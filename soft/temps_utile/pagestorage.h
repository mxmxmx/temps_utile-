#ifndef PAGESTORAGE_H_
#define PAGESTORAGE_H_

/**
 * Helper to save settings in a definable storage (e.g. EEPROM).
 *
 * Divides the available space into fixed-length pages, so we can use a very
 * basic wear-levelling to write pages in a different location each time.
 * EEPROM only has a limited number of (reliable) write cycles so this helps
 * to increase the usable time. Also, the page is only updated when the
 * contents have changed.
 *
 * The "newest" version is identified as the page with the highest generation
 * number; this is an uint32_t so should be safe for a while. A very simple
 * CRC is used to check validity.
 *
 * Note that storage is uninitialized until ::load is called!
 */
template <typename STORAGE, size_t BASE_ADDR, size_t PAGESIZE, typename DATA_TYPE>
class PageStorage {
protected:
 
  /**
   * Binary page structure (aligned to uint16_t for CRC calculation)
   */
  struct page_data {
    uint16_t checksum;
    uint32_t generation;
    uint32_t fourcc;

    DATA_TYPE data;

  } __attribute__((aligned(2)));

public:
  static const size_t PAGES = STORAGE::LENGTH / PAGESIZE;

  // throw compiler error if page size is too small
  typedef bool CHECK_PAGESIZE[sizeof(page_data) > PAGESIZE ? -1 : 1];
  typedef bool CHECK_BASEADDR[BASE_ADDR > STORAGE::LENGTH ? -1 : 1];

  /**
   * @return index of page in storage; only valid after ::load
   */
  int page_index() const {
    return page_index_;
  }

  /**
   * Scan all pages to find the newest to load.
   * If no data found, initialize internal page to empty.
   * @param data [out] loaded data if load successful, else unmodified
   * @return true if data loaded
   */
  bool load(DATA_TYPE &data) {

    page_index_ = -1;
    page_data next_page;
    size_t pages = PAGES;
    while (pages--) {
      int next = (page_index_ + 1) % PAGES;
      STORAGE::read(BASE_ADDR + next * PAGESIZE, &next_page, sizeof(next_page));

      if (DATA_TYPE::FOURCC != next_page.fourcc)
        break;
      if (next_page.checksum != checksum(next_page))
          break;
      if (next_page.generation < page_.generation)
        break;

      page_index_ = next;
      memcpy(&page_, &next_page, sizeof(page_));
    }
    
    if (-1 == page_index_) {
      memset(&page_, 0, sizeof(page_));
      page_.fourcc = DATA_TYPE::FOURCC;
      return false;
    } else {
      memcpy(&data, &page_.data, sizeof(DATA_TYPE));
      return true;
    }
  }

  /**
   * Save data to storage; assumes ::load has been called!
   * @return true if actually stored
   */
  bool save(const DATA_TYPE &data) {

    bool dirty = false;
    const uint8_t *src = (const uint8_t*)&data;
    uint8_t *dst = (uint8_t*)&page_.data;
    size_t length = sizeof(DATA_TYPE);
    while (length--) {
      if (*dst != *src)
        dirty = true;
      *dst++ = *src++;
    }

    if (dirty) {
      ++page_.generation;
      page_.checksum = checksum(page_);
      page_index_ = (page_index_ + 1) % PAGES;

      STORAGE::write(BASE_ADDR + page_index_ * PAGESIZE, &page_, sizeof(page_));
    }

    return dirty;
  }

protected:

  int page_index_;
  page_data page_;

  uint16_t checksum(const page_data &page) {
    uint16_t c = 0;
    const uint16_t *p = (const uint16_t *)&page;
    // Don't include checksum itself in calculation
    ++p;
    size_t length = sizeof(page_data) / sizeof(uint16_t) - sizeof(uint16_t);
    while (length--) {
      c += *p++;
    }

    return c ^ 0xffff;
  }
};

template <uint32_t a, uint32_t b, uint32_t c, uint32_t d>
struct FOURCC
{
  static const uint32_t value = ((a&0xff) << 24) | ((b&0xff) << 16) | ((c&0xff) << 8) | (d&0xff);
};

#endif // PAGESTORAGE_H_

