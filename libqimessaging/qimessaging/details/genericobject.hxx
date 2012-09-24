#pragma once
/*
**  Copyright (C) 2012 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef _QIMESSAGING_DETAILS_GENERICOBJECT_HXX_
#define _QIMESSAGING_DETAILS_GENERICOBJECT_HXX_

#include <qimessaging/buffer.hpp>


namespace qi {

  template<typename T>
  GenericValue makeObjectValue(T* ptr)
  {
    GenericValue res = toValue(ptr).clone();
    qiLogDebug("meta") <<"metaobject on " << ptr <<" " << res.value;
    return res;
  }

  // We need to specialize Type on genericobject to make a copy
  template<> class TypeDefaultClone<GenericObject>
  {
  public:
    static void* clone(void* src)
    {
      GenericObject* osrc = (GenericObject*)src;
      GenericObject* res = new GenericObject(*osrc);
      res->value = res->type->clone(res->value);
      return res;
     }
    static void destroy(void* ptr)
    {
      GenericObject* go = (GenericObject*)ptr;
      go->type->destroy(go->value);
      delete (GenericObject*)ptr;
    }
  };

  namespace detail
  {

    template<typename T> class FutureAdapter
    : public qi::FutureInterface<qi::MetaFunctionResult>
    {
    public:
      FutureAdapter(qi::Promise<T> prom) :prom(prom) {}
      ~FutureAdapter() {}
      virtual void onFutureFinished(const qi::MetaFunctionResult &future, void *data)
      {
        if (future.getMode() == MetaFunctionResult::Mode_GenericValue)
        {
          GenericValue val =  future.getValue();
          typedef std::pair<const T*, bool>  ConvType;
          ConvType resConv = val.template to<T>();
          if (resConv.first)
            prom.setValue(*resConv.first);
          else
            prom.setError("Unable to convert call result to target type");
          if (resConv.second)
            delete const_cast<T*>(resConv.first);
        }
        else
        {
          IDataStream in(future.getBuffer());
          // Not all types are serializable, go through MetaType
          Type* type = typeOf<T>();
          void* ptr = type->deserialize(in);
          if (!ptr)
          {
            prom.setError("Could not deserialize result");
          }
          else
          {
            prom.setValue(*(T*)ptr);
            type->destroy(ptr);
          }
        }
        delete this;
      }
      virtual void onFutureFailed(const std::string &error, void *data)
      {
        prom.setError(error);
        delete this;
      }
      qi::Promise<T> prom;
    };
    template<> class FutureAdapter<void>
    : public qi::FutureInterface<qi::MetaFunctionResult>
    {
    public:
      FutureAdapter(qi::Promise<void> prom) :prom(prom) {}
      ~FutureAdapter() {}
      virtual void onFutureFinished(const qi::MetaFunctionResult &future, void *data)
      {
        prom.setValue(0);
        delete this;
      }
      virtual void onFutureFailed(const std::string &error, void *data)
      {
        prom.setError(error);
        delete this;
      }
      qi::Promise<void> prom;
    };
  }

  template<typename R>
  qi::FutureSync<R> GenericObject::call(const std::string& methodName,
    qi::AutoGenericValue p1,
      qi::AutoGenericValue p2,
      qi::AutoGenericValue p3,
      qi::AutoGenericValue p4,
      qi::AutoGenericValue p5,
      qi::AutoGenericValue p6,
      qi::AutoGenericValue p7,
      qi::AutoGenericValue p8)
  {
    qi::Promise<R> res;
    if (!value || !type) {
      res.setError("Invalid GenericObject");
      return res.future();
    }
    qi::AutoGenericValue* vals[8]= {&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8};
    std::vector<qi::GenericValue> params;
    for (unsigned i=0; i<8; ++i)
      if (vals[i]->value)
        params.push_back(*vals[i]);

    // Signature construction
    std::string signature = methodName + "::(";
    for (unsigned i=0; i< params.size(); ++i)
      signature += params[i].signature();
    signature += ")";
    std::string sigret;
    // Go through MetaType::signature which can return unknown, since
    // signatureFroType will produce a static assert
    sigret = typeOf<R>()->signature();
    // Future adaptation
    // Mark params as being on the stack
    MetaFunctionParameters p(params, true);
    qi::Future<qi::MetaFunctionResult> fmeta = xMetaCall(sigret, signature, p);
    fmeta.addCallbacks(new detail::FutureAdapter<R>(res));
    return res.future();
  }
}
#endif  // _QIMESSAGING_DETAILS_GENERICOBJECT_HXX_
