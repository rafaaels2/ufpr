require 'active_record'

class String
    def only_letters?
        !!match(/[[:alpha:]]+$/)
    end

    def only_numbers?
        !!match(/^\d+$/)
    end
end

ActiveRecord::Base.establish_connection :adapter => "sqlite3",
                                        :database => "Tabelas.sqlite3"

class Carro < ActiveRecord::Base;
    validate :crv_tam_4, :somente_caracteres, :somente_numeros

    def crv_tam_4
        if crv.length != 4
            errors.add(:crv, "# CRV deve que ter 4 algarismos")
        end
    end

    def somente_caracteres
        if !marca.only_letters?
            errors.add(:marca, "# Marca deve ter somente caracteres")
        end
    end

    def somente_numeros
        if !crv.only_numbers?
            errors.add(:crv, "# CRV deve ter somente números")
        end
    end
end