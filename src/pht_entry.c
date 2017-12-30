/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Thomas Punt <tpunt@php.net>                                  |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <main/php.h>
#include <Zend/zend_closures.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_smart_str.h>
#include <ext/standard/php_var.h>

#include "php_pht.h"
#include "src/pht_entry.h"
#include "src/pht_copy.h"

void pht_entry_delete(void *entry_void)
{
    pht_entry_t *entry = entry_void;

    pht_entry_delete_value(entry);

    free(entry);
}

void pht_entry_delete_value(pht_entry_t *entry)
{
    switch (PHT_ENTRY_TYPE(entry)) {
        case PHT_STORE_FUNC:
            free(PHT_ENTRY_FUNC(entry));
            break;
        case IS_ARRAY:
        case IS_STRING:
            free(PHT_STRV(PHT_ENTRY_STRING(entry)));
    }
}

void pht_convert_entry_to_zval(zval *value, pht_entry_t *e)
{
    switch (PHT_ENTRY_TYPE(e)) {
        case IS_STRING:
            ZVAL_STR(value, zend_string_init(PHT_STRV(PHT_ENTRY_STRING(e)), PHT_STRL(PHT_ENTRY_STRING(e)), 0));
            break;
        case IS_LONG:
            ZVAL_LONG(value, PHT_ENTRY_LONG(e));
            break;
        case IS_DOUBLE:
            ZVAL_DOUBLE(value, PHT_ENTRY_DOUBLE(e));
            break;
        case _IS_BOOL:
            ZVAL_BOOL(value, PHT_ENTRY_BOOL(e));
            break;
        case IS_TRUE:
            ZVAL_TRUE(value);
            break;
        case IS_FALSE:
            ZVAL_FALSE(value);
            break;
        case IS_NULL:
            ZVAL_NULL(value);
            break;
        case IS_ARRAY:
            {
                size_t buf_len = PHT_STRL(PHT_ENTRY_STRING(e));
                const unsigned char *p = (const unsigned char *) PHT_STRV(PHT_ENTRY_STRING(e));
                php_unserialize_data_t var_hash;

                PHP_VAR_UNSERIALIZE_INIT(var_hash);

                if (!php_var_unserialize(value, &p, p + buf_len, &var_hash)) {
                    // @todo handle serialisation failure - is this even possible to hit?
                }

                PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
            }
            break;
        case PHT_STORE_FUNC:
            {
                zend_function *closure = copy_user_function(PHT_ENTRY_FUNC(e), NULL);
                char *name;
                size_t name_len;

                zend_create_closure(value, closure, zend_get_executed_scope(), closure->common.scope, NULL);
                name_len = spprintf(&name, 0, "Closure@%p", zend_get_closure_method_def(value));

                if (!zend_hash_str_update_ptr(EG(function_table), name, name_len, closure)) {
                    printf("FAILED!\n");
                }

                efree(name);
            }
            break;
        case PHT_QUEUE:
            {
                zend_string *ce_name = zend_string_init("Queue", sizeof("Queue") - 1, 0);
                zend_class_entry *ce = zend_lookup_class(ce_name);
                zval zobj;

                PHT_ZG(skip_qoi_creation) = 1;

                if (object_init_ex(&zobj, ce) != SUCCESS) {
                    // @todo this will throw an exception in the new thread, rather than at
                    // the call site - how should it behave?
                    zend_throw_exception(zend_ce_exception, "Failed to threaded object from Queue class", 0);
                }

                PHT_ZG(skip_qoi_creation) = 0;

                queue_obj_t *new_qo = (queue_obj_t *)((char *)Z_OBJ(zobj) - Z_OBJ(zobj)->handlers->offset);

                new_qo->qoi = PHT_ENTRY_Q(e)->qoi;

                zend_string_free(ce_name);

                ZVAL_OBJ(value, Z_OBJ(zobj));
            }
            break;
        case PHT_HASH_TABLE:
            {
                zend_string *ce_name = zend_string_init("HashTable", sizeof("HashTable") - 1, 0);
                zend_class_entry *ce = zend_lookup_class(ce_name);
                zval zobj;

                PHT_ZG(skip_htoi_creation) = 1;

                if (object_init_ex(&zobj, ce) != SUCCESS) {
                    // @todo this will throw an exception in the new thread, rather than at
                    // the call site - how should it behave?
                    zend_throw_exception(zend_ce_exception, "Failed to threaded object from HashTable class", 0);
                }

                PHT_ZG(skip_htoi_creation) = 0;

                hashtable_obj_t *new_hto = (hashtable_obj_t *)((char *)Z_OBJ(zobj) - Z_OBJ(zobj)->handlers->offset);

                new_hto->htoi = PHT_ENTRY_HT(e)->htoi;

                zend_string_free(ce_name);

                ZVAL_OBJ(value, Z_OBJ(zobj));
            }
            break;
        case PHT_VECTOR:
            {
                zend_string *ce_name = zend_string_init("Vector", sizeof("Vector") - 1, 0);
                zend_class_entry *ce = zend_lookup_class(ce_name);
                zval zobj;

                PHT_ZG(skip_voi_creation) = 1;

                if (object_init_ex(&zobj, ce) != SUCCESS) {
                    // @todo this will throw an exception in the new thread, rather than at
                    // the call site - how should it behave?
                    zend_throw_exception(zend_ce_exception, "Failed to threaded object from Vector class", 0);
                }

                PHT_ZG(skip_voi_creation) = 0;

                vector_obj_t *new_vo = (vector_obj_t *)((char *)Z_OBJ(zobj) - Z_OBJ(zobj)->handlers->offset);

                new_vo->voi = PHT_ENTRY_V(e)->voi;

                zend_string_free(ce_name);

                ZVAL_OBJ(value, Z_OBJ(zobj));
            }
            break;
        case IS_OBJECT:
            {
                size_t buf_len = PHT_STRL(PHT_ENTRY_STRING(e));
                const unsigned char *p = (const unsigned char *) PHT_STRV(PHT_ENTRY_STRING(e));
                php_unserialize_data_t var_hash;

                PHP_VAR_UNSERIALIZE_INIT(var_hash);

                if (!php_var_unserialize(value, &p, p + buf_len, &var_hash)) {
                    // @todo handle serialisation failure - is this even possible to hit?
                }

                PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
            }
    }
}

void pht_convert_zval_to_entry(pht_entry_t *e, zval *value)
{
    PHT_ENTRY_TYPE(e) = Z_TYPE_P(value);

    switch (Z_TYPE_P(value)) {
        case IS_STRING:
            PHT_STRL(PHT_ENTRY_STRING(e)) = ZSTR_LEN(Z_STR_P(value));
            PHT_STRV(PHT_ENTRY_STRING(e)) = malloc(sizeof(char) * PHT_STRL(PHT_ENTRY_STRING(e)));
            memcpy(PHT_STRV(PHT_ENTRY_STRING(e)), ZSTR_VAL(Z_STR_P(value)), sizeof(char) * PHT_STRL(PHT_ENTRY_STRING(e)));
            break;
        case IS_LONG:
            PHT_ENTRY_LONG(e) = Z_LVAL_P(value);
            break;
        case IS_DOUBLE:
            PHT_ENTRY_DOUBLE(e) = Z_DVAL_P(value);
            break;
        case _IS_BOOL:
            PHT_ENTRY_BOOL(e) = !!Z_LVAL_P(value);
            break;
        case IS_TRUE:
            PHT_ENTRY_BOOL(e) = 1;
            break;
        case IS_FALSE:
            PHT_ENTRY_BOOL(e) = 0;
            break;
        case IS_NULL:
            break;
        case IS_ARRAY:
            {
                smart_str smart = {0};
                php_serialize_data_t vars;

                PHP_VAR_SERIALIZE_INIT(vars);
                php_var_serialize(&smart, value, &vars);
                PHP_VAR_SERIALIZE_DESTROY(vars);

                if (EG(exception)) {
                    smart_str_free(&smart);
                    PHT_ENTRY_TYPE(e) = PHT_SERIALISATION_FAILED;
                } else {
                    zend_string *sval = smart_str_extract(&smart);

                    PHT_STRL(PHT_ENTRY_STRING(e)) = ZSTR_LEN(sval);
                    PHT_STRV(PHT_ENTRY_STRING(e)) = malloc(ZSTR_LEN(sval));
                    memcpy(PHT_STRV(PHT_ENTRY_STRING(e)), ZSTR_VAL(sval), ZSTR_LEN(sval));

                    zend_string_free(sval);
                }
            }
            break;
        case IS_OBJECT:
            {
                if (instanceof_function(Z_OBJCE_P(value), zend_ce_closure)) {
                    PHT_ENTRY_TYPE(e) = PHT_STORE_FUNC;
                    PHT_ENTRY_FUNC(e) = malloc(sizeof(zend_op_array));
                    memcpy(PHT_ENTRY_FUNC(e), zend_get_closure_method_def(value), sizeof(zend_op_array));
                    Z_ADDREF_P(value);
                } else if (instanceof_function(Z_OBJCE_P(value), Queue_ce)) {
                    queue_obj_t *qo = (queue_obj_t *)((char *)Z_OBJ_P(value) - Z_OBJ_P(value)->handlers->offset);

                    PHT_ENTRY_TYPE(e) = PHT_QUEUE;
                    PHT_ENTRY_Q(e) = qo;

                    pthread_mutex_lock(&qo->qoi->lock);
                    ++qo->qoi->refcount;
                    pthread_mutex_unlock(&qo->qoi->lock);
                } else if (instanceof_function(Z_OBJCE_P(value), HashTable_ce)) {
                    hashtable_obj_t *hto = (hashtable_obj_t *)((char *)Z_OBJ_P(value) - Z_OBJ_P(value)->handlers->offset);

                    PHT_ENTRY_TYPE(e) = PHT_HASH_TABLE;
                    PHT_ENTRY_HT(e) = hto;

                    pthread_mutex_lock(&hto->htoi->lock);
                    ++hto->htoi->refcount;
                    pthread_mutex_unlock(&hto->htoi->lock);
                }  else if (instanceof_function(Z_OBJCE_P(value), Vector_ce)) {
                    vector_obj_t *vo = (vector_obj_t *)((char *)Z_OBJ_P(value) - Z_OBJ_P(value)->handlers->offset);

                    PHT_ENTRY_TYPE(e) = PHT_VECTOR;
                    PHT_ENTRY_V(e) = vo;

                    pthread_mutex_lock(&vo->voi->lock);
                    ++vo->voi->refcount;
                    pthread_mutex_unlock(&vo->voi->lock);
                } else {
                    // temporary solution - just serialise it and to the hell with the consequences
                    smart_str smart = {0};
                    php_serialize_data_t vars;

                    PHP_VAR_SERIALIZE_INIT(vars);
                    php_var_serialize(&smart, value, &vars);
                    PHP_VAR_SERIALIZE_DESTROY(vars);

                    if (EG(exception)) {
                        smart_str_free(&smart);
                        PHT_ENTRY_TYPE(e) = PHT_SERIALISATION_FAILED;
                    } else {
                        zend_string *sval = smart_str_extract(&smart);

                        PHT_STRL(PHT_ENTRY_STRING(e)) = ZSTR_LEN(sval);
                        PHT_STRV(PHT_ENTRY_STRING(e)) = malloc(ZSTR_LEN(sval));
                        memcpy(PHT_STRV(PHT_ENTRY_STRING(e)), ZSTR_VAL(sval), ZSTR_LEN(sval));

                        zend_string_free(sval);
                    }
                }
            }
            break;
        default:
            PHT_ENTRY_TYPE(e) = PHT_SERIALISATION_FAILED;
    }
}

void pht_entry_update(pht_entry_t *entry, zval *value)
{
    pht_entry_delete_value(entry);
    pht_convert_zval_to_entry(entry, value);
}

pht_entry_t *create_new_entry(zval *value)
{
    pht_entry_t *e = malloc(sizeof(pht_entry_t));

    pht_convert_zval_to_entry(e, value);

    return e;
}