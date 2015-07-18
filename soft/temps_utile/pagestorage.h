#ifndef PAGESTORAGE_H_
#define PAGESTORAGE_H_

/**
 * Helper to save settings (in EEPROM)
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
template <size_t _BASE, size_t _LENGTH, size_t _PAGESIZE, typename _TYPE>
class PageStorage {
protected:
 
  /**
   * Binary page structure (aligned to uint16_t for CRC calculation)
   */
  struct page_data {
    uint16_t checksum;
    uint32_t generation;
    uint32_t id;

    _TYPE data;

  } __attribute__((aligned(2)));

public:
  static const size_t BASE_ADDR = _BASE;
  static const size_t LENGTH = _LENGTH;
  static const size_t PAGESIZE = _PAGESIZE;
  static const size_t PAGES = LENGTH / PAGESIZE;
  static const uint32_t TYPEID = _TYPE::ID;
  typedef _TYPE TYPE;
  // throw compiler error if page size is too small
  typedef page_data CHECK[ sizeof( page_data ) > PAGESIZE ? -1 : 1 ];

  int page_index() const {
    return _page_index;
  }

  /**
   * Scan all pages to find the newest to load.
   *
   * @return true if data loaded
   */
  bool load( TYPE &_data ) {

    _page_index = -1;
    page_data next_page;
    size_t pages = PAGES;
    while ( pages-- ) {
      int next = ( _page_index + 1 ) % PAGES;
      EEPROM.get( BASE_ADDR + next * PAGESIZE, next_page );

      if ( TYPEID != next_page.id )
        break;
      if ( next_page.checksum != checksum( next_page ) )
          break;
      if ( next_page.generation < page.generation )
        break;

      _page_index = next;
      memcpy( &page, &next_page, sizeof( page ) );
    }
    
    if ( -1 == _page_index ) {
      memset( &page, 0, sizeof( page ) );
      page.id = TYPEID;
      return false;
    } else {
      memcpy( &_data, &page.data, sizeof( TYPE ) );
      return true;
    }
  }

  /**
   * Save data to storage; assumes ::load has been called!
   * @return true if actually stored
   */
  bool save( const TYPE &_data ) {

    bool dirty = false;
    const uint8_t *src = (const uint8_t*)&_data;
    uint8_t *dst = (uint8_t*)&page.data;
    size_t length = sizeof( TYPE );
    while ( length-- ) {
      dirty |= *dst != *src;
      *dst++ = *src++;
    }

    if ( dirty ) {
      ++page.generation;
      page.checksum = checksum( page );
      _page_index = ( _page_index + 1 ) % PAGES;

      EEPROM.put( BASE_ADDR + _page_index * PAGESIZE, page );
    }

    return dirty;
  }

protected:

  int _page_index;
  page_data page;

  uint16_t checksum( const page_data &_page ) {
    uint16_t c = 0;
    const uint16_t *p = (const uint16_t *)&_page;
    // Don't include checksum itself in calculation
    ++p;
    size_t length = sizeof( page_data ) / sizeof( uint16_t ) - sizeof( uint16_t );
    while ( length-- ) {
      c += *p++;
    }

    return c ^ 0xffff;
  }
};

template <uint32_t _1, uint32_t _2, uint32_t _3, uint32_t _4>
struct FOURCC
{
  static const uint32_t value = ((_1&0xff) << 24) | ((_2&0xff) << 16) | ((_3&0xff) << 8) | (_4&0xff);
};

#endif // PAGESTORAGE_H_

