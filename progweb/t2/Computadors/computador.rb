require 'active_record'

class String
    def only_letters?
        !!match(/[[:alpha:]]+$/)
    end
end

ActiveRecord::Base.establish_connection :adapter => "sqlite3",
                                        :database => "Tabelas.sqlite3"

class Computador < ActiveRecord::Base;
    belongs_to :pessoa
    
    validate :numSerie_tam_4, :somente_caracteres, 

    def numSerie_tam_4
        if numSerie.length != 4
            errors.add(:numSerie, "# Número de Sérire deve que ter 4 algarismos")
        end
    end

    def somente_caracteres
        if !marca.only_letters?
            errors.add(:marca, "# Marca deve ter somente caracteres")
        end
    end
end