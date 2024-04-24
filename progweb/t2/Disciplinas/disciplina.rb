require 'active_record'

ActiveRecord::Base.establish_connection :adapter => "sqlite3",
                                        :database => "Tabelas.sqlite3"

class Disciplina < ActiveRecord::Base;
    has_and_belongs_to_many :pessoas   
    
    validate :codigo_tam_4

    def codigo_tam_4
        if codigo.length != 4
            errors.add(:codigo, "# Código deve que ter 4 algarismos")
        end
    end
end