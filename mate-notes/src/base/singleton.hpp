


#ifndef __BASE_SINGLETON_HPP__
#define __BASE_SINGLETON_HPP__


namespace base {


  template <class _Type>
  class Singleton 
  {
  public:
    static _Type & obj()
      {
        // TODO make this thread safe;
        static _Type * instance = new _Type();
        return *instance;
      }

  };

}


#endif
